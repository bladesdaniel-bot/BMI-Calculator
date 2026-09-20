# 🚀 BMI Calculator & Hidden Sci-Fi Arcade

https://github.com/bladesdaniel-bot/BMI-Calculator/blob/main/BMI%20Calculator%20Video/Bmi%20Calculator%20Full%20Demo.mp4

A custom native Windows desktop application that serves as a fully functional Body Mass Index (BMI) calculator, wrapped in a sleek, dark sci-fi user interface. 

However, with the click of a button, the application seamlessly transforms into a **3D warp-speed space shooter**, featuring interactive flight controls, planetary collision detection, and multi-stage explosions—all built completely from scratch without a game engine.

![BMI Calculator Demo](BMI%20Calculator%20Video/BMI%20Calculator.jpg)

*(Check out the `BMI Calculator Video` folder in this repository for full video demonstrations of the app and arcade mode in action!)*

## 🛠️ Features

### 💻 The Calculator
* **Real-Time Calculation:** Instantly calculates BMI as the user types.
* **Unit Toggling:** Seamlessly switch between Metric (kg/cm) and Imperial (lbs/ft/in) systems.
* **Dynamic UI:** Color-coded health categories (Underweight, Normal, Overweight, Obese) based on the calculated result.
* **Custom Frameless Design:** Bypasses the default Windows DWM frame to utilize a completely custom borderless window with dynamic minimize/maximize/close buttons.

### 🌌 The Arcade Mode
* **3D Starfield Engine:** A custom-built depth-rendering engine that simulates warp speed.
* **Interactive Cockpit Controls:** Features a drag-and-drop virtual flight joystick and an adjustable throttle slider that actively scales game speed.
* **Combat System:** On-screen fire button, laser cooldowns, and collision detection.
* **Dynamic Rendering:** Procedurally generated celestial bodies, alien saucers, and multi-layered explosion animations rendered entirely via code.

## ⚙️ Tech Stack
This project was built entirely in **C++** using the native **Win32 API**. 
* **Graphics:** Native Windows GDI (Graphics Device Interface) for all custom drawing, shapes, UI elements, and frame generation.
* **Window Management:** Custom `WM_PAINT`, `WM_NCCALCSIZE`, and `WM_NCHITTEST` handling for the custom title bar and borderless application frame.
* **Development Environment:** Visual Studio.

## 🎮 How to Play (Arcade Mode)
1. Launch the application.
2. Click the **Arcade Mode** button in the custom title bar.
3. Use the **Virtual Joystick** (bottom right) to steer your ship.
4. Adjust the **Throttle** (far right) to speed up or slow down.
5. Tap the **Fire** button (or press `SPACEBAR`) to shoot lasers.
6. Destroy planets and alien saucers for points, but watch your Hull Integrity!

## 🚀 How to Build and Run
1. Clone this repository to your local machine.
2. Open the `.sln` file in **Visual Studio**.
3. Ensure the build configuration is set to **Release** (x64 or x86).
4. Click **Build Solution**.
5. Run the compiled `.exe` file.
