# Bifractalizer
 An audio plugin that fractalizes and defractalizes sound 😺
 
![image](https://github.com/user-attachments/assets/b36a45b7-91fd-4b40-98c6-3c124ebfa112)

## Video example:



https://github.com/user-attachments/assets/28e6bd55-8942-42ca-83df-090278376e3e




## How to build:
1) Install JUCE, some cpp compiler and CMake. 
2) Run `git clone --recursive https://github.com/Ankalot/Bifractalizer.git` to get source code.
3) Run `build_first_time.bat`. For the next builds use `rebuild.bat`. You can also build a release version (`build_release.bat`).
4) Now you have VST3 and Standalone versions. If you want other types, then change the settings in cmake.
  
## How to debug:
To debug the program through VS Code and any DAW, configure the `.vscode/launch.json` file.
