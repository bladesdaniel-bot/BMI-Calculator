# Win32 C++ BMI Calculator

A lightweight, standalone desktop Body Mass Index (BMI) calculator built entirely in C++ using the native Windows (Win32) API. 

This application features a custom-styled graphical user interface (GUI) designed with a high-contrast aesthetic: a sleek black background, lime green static labels, red text input fields, and vibrant deep sky blue output text. Users simply enter their weight in pounds and height in feet and inches to instantly compute their BMI and view their health category.

## Features
* **No Dependencies:** Built with native Win32 libraries, requiring no external frameworks like Qt or .NET.
* **Custom GUI Styling:** High-contrast color scheme utilizing `WM_CTLCOLORSTATIC` and `WM_CTLCOLOREDIT` message handling.
* **Portable:** Compiles down to a single, standalone `.exe` file. No installation wizard required.
* **Input Validation:** Gracefully handles invalid inputs and prevents crashes using `std::stod` and `try-catch` blocks.

## How to Download and Run (End Users)

Because this is a standalone executable, it is completely portable and requires no installation.

**1. Download the Application**
* Navigate to the **Releases** section on the right side of this repository.
* Download the compressed `BMICalculator.zip` file and extract it to your Desktop or Downloads folder.

**2. Bypass the Windows SmartScreen Warning**
* Because this is a custom-built independent application, Windows Defender might flag it as an "unrecognized app" the first time you open it. 
* If a blue "Windows protected your PC" popup appears, click the **More info** link just beneath the main warning text.
* Click the **Run anyway** button that appears at the bottom of the window.

**3. Launch the Calculator**
* Double-click the **`BMICalculator.exe`** icon to launch the application instantly. 

## Building from Source (Developers)

If you want to compile the source code yourself, you will need the Microsoft C++ compiler (MSVC). 

**Option 1: Using Developer Command Prompt**
1. Open the **Developer Command Prompt for VS**.
2. Clone this repository and navigate to the project folder:
   ```cmd
   git clone [https://github.com/YourUsername/YourRepositoryName.git](https://github.com/YourUsername/YourRepositoryName.git)
   cd YourRepositoryName
   ```
3. Compile the code by linking the required Windows libraries (`user32.lib` and `gdi32.lib`):
   ```cmd
   cl /EHsc "BMI Calculator.cpp" user32.lib gdi32.lib
   ```
4. Run the newly generated executable:
   ```cmd
   "BMI Calculator.exe"
   ```

**Option 2: Using Visual Studio**
1. Open Visual Studio and select **Create a new project**.
2. Choose the **Empty Project (C++)** template.
3. Add a new `.cpp` file to your Source Files and paste the code from `BMI Calculator.cpp`.
4. Ensure your build configuration is set to **Release / x64**.
5. Click **Build Solution** (Ctrl+Shift+B).

