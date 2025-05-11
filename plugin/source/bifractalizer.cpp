#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <numbers>

#include <juce_audio_processors/juce_audio_processors.h>

#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

//using FloatSolver = Eigen::SparseLU<Eigen::SparseMatrix<float>>;
using FloatSolver = Eigen::BiCGSTAB<Eigen::SparseMatrix<float>>;


namespace audio_plugin {
std::vector<float> linspace(float start, float end, int num, bool endpoint = false) {
    std::vector<float> result(num);
    float step = (end - start) / (endpoint ? (num - 1) : num);
    for (int i = 0; i < num; ++i) {
        result[i] = start + i * step;
    }
    return result;
}


// f(x) = \sum_{n = 0}^{max_terms}{0.5^n g(\{2^n x\})}
void compute_f_optimized(juce::AudioBuffer<float>& f,        // size(f) == size(x) == size(g) = pow of 2
                        const std::vector<float>& x,         // size(f) == size(x) == size(g) = pow of 2
                        const juce::AudioBuffer<float>& g,   // size(f) == size(x) == size(g) = pow of 2
                        const std::vector<float>& two_pow_n, // size(two_pow_n) == max_terms
                        const std::vector<float>& weights,   // size(weights) == max_terms
                        int max_terms = 20                   /* Must be divisible by 4!*/) {
    const int N = g.getNumSamples();
    const int numChannels = f.getNumChannels();
    const float inv_g_step = static_cast<float>(N);
    const float* x_data = x.data();
    const int mod_mask = N - 1; // Power-of-2 fast modulo

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* g_data = g.getReadPointer(ch);
        float* f_data = f.getWritePointer(ch);

        for (int i = 0; i < N; ++i) {
            const float xi = x_data[i];
            float sum = 0.0f;

            for (int n = 0; n < max_terms; n += 4) {
                const float arg0 = xi * two_pow_n[n];
                const float arg1 = xi * two_pow_n[n+1];
                const float arg2 = xi * two_pow_n[n+2];
                const float arg3 = xi * two_pow_n[n+3];

                const int idx0 = static_cast<int>((arg0 - std::floor(arg0)) * inv_g_step + 0.5f) & mod_mask;
                const int idx1 = static_cast<int>((arg1 - std::floor(arg1)) * inv_g_step + 0.5f) & mod_mask;
                const int idx2 = static_cast<int>((arg2 - std::floor(arg2)) * inv_g_step + 0.5f) & mod_mask;
                const int idx3 = static_cast<int>((arg3 - std::floor(arg3)) * inv_g_step + 0.5f) & mod_mask;

                sum += weights[n]   * g_data[idx0];
                sum += weights[n+1] * g_data[idx1];
                sum += weights[n+2] * g_data[idx2];
                sum += weights[n+3] * g_data[idx3];
            }

            f_data[i] = sum;
        }
    }
}


/*void compute_f(juce::AudioBuffer<float>& f,
               const std::vector<float>& x,
               const juce::AudioBuffer<float>& g,
               const std::vector<float>& two_pow_n,
               const std::vector<float>& weights,
               int max_terms = 20) {
    const int N = g.getNumSamples();
    const int numChannels = f.getNumChannels();

    const float inv_g_step = static_cast<float>(N);
    const float* x_data = x.data();

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* g_data = g.getReadPointer(ch);
        float* f_data = f.getWritePointer(ch);

        for (int i = 0; i < N; ++i) {
            float sum = 0.0f;
            const float xi = x_data[i];

            for (int n = 0; n < max_terms; ++n) {
                const float arg = xi * two_pow_n[n];
                const float fractional = arg - std::floor(arg);
                const int idx = static_cast<int>(fractional * inv_g_step + 0.5f);
                const int safe_idx = idx < N ? idx : N - 1;

                sum += weights[n] * g_data[safe_idx];
            }

            f_data[i] = sum;
        }
    }
}*/


void fractalize(const std::vector<float> &x_grid, 
                const juce::AudioBuffer<float> &g, 
                juce::AudioBuffer<float> &f,
                const std::vector<float> &two_pow_n, 
                const std::vector<float> &weights,
                int max_terms = 20) {
    f.clear();

    compute_f_optimized(f, x_grid, g, two_pow_n, weights, max_terms);
}


void findDefractalizerMatrix(Eigen::SparseMatrix<float>& A,
                             const std::vector<float> &two_pow_n,
                             const std::vector<float> &weights,
                             int N, int max_terms = 20) {
    A.resize(N, N);
    A.reserve(Eigen::VectorXi::Constant(N, max_terms));

    const float dx = 1.0 / N;
    
    for (int i = 0; i < N; ++i) {
        const float x = i * dx;
        for (int n = 0; n < max_terms; ++n) {
            const float arg = two_pow_n[n] * x;
            const int j = static_cast<int>((arg - std::floor(arg)) * N + 0.5f) % N;
            
            A.coeffRef(i, j) += weights[n];
        }
    }
    
    A.makeCompressed();
}


// f(x) = \sum_{n = 0}^{max_terms}{0.5^n g(\{2^n x\})}
// finding g(x) from f(x) by solving system of linear equations
void defractalize(const juce::AudioBuffer<float> &f,
                  juce::AudioBuffer<float> &g,
                  const Eigen::SparseMatrix<float>& A,
                  FloatSolver& solver) {
    const int N = g.getNumSamples();
    const int numChannels = g.getNumChannels();
    
    Eigen::VectorXf fEigen(N);
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* f_data = f.getReadPointer(ch);
        for (int i = 0; i < N; ++i) {
            fEigen(i) = f_data[i];
        }

        Eigen::VectorXf gEigen = solver.solve(fEigen);

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Factorization failed");
        }
        
        float* g_data = g.getWritePointer(ch);
        for (int i = 0; i < N; ++i) {
            g_data[i] = gEigen(i);
        }
    }
}
}