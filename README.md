# Bifractalizer
 An audio plugin that fractalizes and defractalizes sound 😺
 
![image](https://github.com/user-attachments/assets/b36a45b7-91fd-4b40-98c6-3c124ebfa112)

## How to build:
1) Install JUCE and some cpp compiler. 
2) Download Eigen C++ library and place it in `/libs` folder.
3) Run `build_first_time.bat`. For the next builds use `rebuild.bat`.
4) Now you have VST3 and Standalone versions. If you want other types, then change the settings in cmake.
  
## How to debug:
To debug the program through VS Code and any DAW, configure the `.vscode/launch.json` file.
