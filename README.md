# 🎉 NOW YOU CAN CHANGE COLOR FOR PROP_STATIC! 🎉

✨ I created a special tool that allow you to apply color to prop_static, similar to what is available in CS:GO! It will be very useful for those who create modifications based on the Source SDK 2013!

Advantages:

- No need to manually repaint assets, just change the color in Hammer++!
- The tool does not recompile each model, which has a positive effect on speed!
- Detailed log that will notify you about each stage!

<picture>
  <source srcset="images/propcolor-fire.gif">
</picture>

# INSTALLATION:
1. Drag the PropColorCompiler.exe and FGD file to the folder "Source SDK Base 2013 Singleplayer\bin\"
2. Add the FGD file to the configuration via `Tools -> Options -> Add`
3. Open the compilation presets via `Run Map! -> Expert...`
4. You need to create two new commands on the left side of the window, in the list, the first one should be located at the very top before starting the entire compilation! The second one should be below `$light_exe` and above `Copy File!`
5. In both commands, on the right in the Command column, we write the full path to the tool (for example `H:\SteamLibrary\steamapps\common\Source SDK Base 2013 Singleplayer\bin\PropColorCompiler.exe`)
6. In the Parameters column for the first command, write `$path\$file.$ext $gamedir` for the second is `-postcompile $path\$file.$ext $gamedir`

It will look like this:
<picture>
  <source srcset="images/1.png">
</picture>
<picture>
  <source srcset="images/2.png">
</picture>

8. The tool is fully installed and now you can use it!

## Known issues:
- It is not recommended to compile the map together with the open game. In some cases, this leads to the fact that the passes will be displayed incorrectly!
- There is a standard limit on the number of skins per model for painting. That is, one model can only be painted in 32 colors (white {255 255 255} the standard color is not taken into account!)*

> *This restriction does not apply to all cards, but to each one individually! If you have compiled a map with 32 props of different colors, then on the next one you will also be able to create 32 props with different colors!
Subsequently, a video clip will be released on how to install the tool correctly. The source code will most likely be available only after the release of my project. 

## Special thanks:
- ugo_zapad - code tips, powerful support during development
- Ambiabstract - mastermind [who created a prop scale solution](https://github.com/Ambiabstract/props_scaling_recompiler )
- MyCbEH - mastermind [who created the incredible locations of my favorite mods](https://store.steampowered.com/search/?developer=SnowDropEscape+development+team )
