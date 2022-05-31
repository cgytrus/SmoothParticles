# Smooth Particles
Interpolates particle spawn positions between frames to make the game appear smoother in general

Especially noticeable at higher speeds and/or lower FPS and/or in texture packs with increased particle count

## Examples
### 4x speed ship, 72 FPS
<details>
 <summary>Before</summary>
 
 ![Before](https://cdn.discordapp.com/attachments/504366353965121589/981227330385100870/unknown.png)
</details>
<details>
 <summary>After</summary>
 
 ![After](https://cdn.discordapp.com/attachments/504366353965121589/981229308305608714/unknown.png)
</details>

### 4x speed straight fly in Born Survivor by Splinter25, 72 FPS
<details>
 <summary>Before</summary>
 
 ![Before](https://cdn.discordapp.com/attachments/504366353965121589/981228293720256603/unknown.png)
</details>
<details>
 <summary>After</summary>
 
 ![After](https://cdn.discordapp.com/attachments/504366353965121589/981230521902313472/unknown.png)
</details>

## Installation
Same as any of the [Mat's mods](https://matcool.github.io/mods#install-instructions)

## Compiling
- Clone the repo recursively (with `--recursive`)
- Configure CMake (`cmake -G "Visual Studio 16 2019" -B build -DCMAKE_BUILD_TYPE=Release -T host=x86 -A win32`)
- Build (`cmake --build build --config Release --target ALL_BUILD`)
