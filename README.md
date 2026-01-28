# Nova64_OS  
![Version](https://img.shields.io/badge/version-devV0.1--Alpha-blue)
![Platform](https://img.shields.io/badge/platform-x86__32-lightblue)
![C](https://img.shields.io/badge/Kernel_language-C-blue)
![Last Commit](https://img.shields.io/github/last-commit/SauloHS/Nova64_OS)
![Repo Size](https://img.shields.io/github/repo-size/SauloHS/Nova64_OS)
![License](https://img.shields.io/github/license/SauloHS/Nova64_OS)

## What is Nova64_OS?
**Nova64** is an operational system, made for hobby and studies. It's main target is to be responsible, optimized and useful for very basic tasks.

## Current resources
* **Bootloader**
  * Coded in assembly, a basic bootloader and GDT to jump from real mode (16 bits) to protected mode (32 bits).
* **Drivers**
  * RTC
    * A basic RTC (Real Time Clock) driver with adjustables time zones.
  * VGA
    * A video driver for text mode (80x25) with some basic functions, such as print, putc and print_int.
  * Keyboard
    * A keyboard driver with a basic translation table.
* **Shell**
  * A basic shell with 4 commands.
* **Kernel**
  * A x86_32 Monolithic kernel that handles all the drivers.

## How to compile
1. Clone the repository: ```git clone https://github.com/SauloHS/Nova64_OS.git```
2. To compile, you need to install the following packages: `nasm`, `gcc multilib`, `g++-multilib`, `binutils`, `build-essential`, `qemu-system-x86` and `make`, which can be easyly installed via the following code:
3. ```bash
   sudo apt update && sudo apt install -y \
    build-essential \
    nasm \
    gcc-multilib \
    g++-multilib \
    binutils \
    qemu-system-x86 \
    make
   ```
   **THIS CODE ONLY WORKS FOR LINUX!** If you want to see how to compile in windows, see [wsl](#wsl).
4. After installing all of the packages, you can compile by running ```make``` in the root directory. You can add ```run``` to, after compiling, automatically open QEMU with the image generated, and ```clean``` to remove any binary file, except the image, resulted by the compilation process. I recommend you to use ```make run clean```.

## Why isn't the OS working on my computer?
In emulators, the OS **will work for sure**, if you use the latest commit. In real hardwares, the chances of it not working are quite high. The most common causes aren't your fault, and the only thing you can do is change the source code (that's what I'm doing).
# The Most Common Causes:
* BIOS x UEFI
  * For reasons of ease and simplicity our actual bootloader uses BIOS. The most recent computers (starting in the 2000s) uses UEFI. Many of them, don't have `Legacy Support` or `CSM`, so they can't run BIOS code.
* Keyboard and video drivers
  * Many modern laptops uses keyboards connected via I2C bus or USB, and not the PS/2 controllers the OS supports.
  * If your computer doesn't support standard text mode (VGA 80x25), you probably will see a black screnn or visual trash.

# How do I know if my PC is compatible?
The only way you can know for sure your PC is compatible, is by running the OS. But, there are 2 things you can do:
1. If you use windows, you **need** to disable `Secure Boot`. It is an option in the BIOS of your computer that don't allow any OS unsigned by Microsoft boot in your computer.
2. You can check if your PC has `Legacy support` or `CSM`. If not, we have a bad news for you.

## WSL

If you use **Windows**, we don't recommend you trying to download any windows versions of the linux packages shown in [How to compile](#how-to-compile). We do recommend you to use WSL.

WSL is Windows Subsystem for Linux. You can install following this:
1. Search for "Enable or disable Windows features" in the search bar, and enable "Virtual Machine Platform" and "Windows Subsystem for Linux". Then, restart your computer.
2. Again, in the search bar, search for "Powershell". Click on "Open as an Administrator", and run the following command: `wsl --install`. It will install ubuntu in your machine. Complete the initial setup, and you can compile using the tutorial.

