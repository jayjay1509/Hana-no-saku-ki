## 🌊 2D Fluid Simulation using Cellular Automata

This project is a real-time 2D fluid simulation developed in C++.  
It is based on the principles of **cellular automata**, where each cell in a grid follows simple rules to simulate fluid-like behavior.

### 🚀 Features

- 🔹 Real-time simulation
- 🔹 Grid-based system (cellular automata)
- 🔹 Fluid-like movement (gravity, flow, spreading)
- 🔹 Lightweight and CPU-based implementation
- 🔹 Simple and scalable architecture

### 🧠 How it works

The simulation divides the world into a 2D grid.  
Each cell contains a quantity of "fluid" and updates its state based on its neighbors.

Basic rules include:
- Gravity-driven downward flow
- Horizontal spreading when blocked
- Pressure balancing between cells

Despite the simplicity of these rules, complex and natural fluid behaviors emerge.

### 🛠️ Technologies

- C++
- SFML (for rendering)
- Custom simulation logic

### 📈 Goals

- Explore fluid simulation techniques
- Compare cellular automata with other methods (Navier-Stokes, SPH)
- Optimize for real-time performance
- Potential GPU acceleration (future work)

### 📚 Inspiration

This project is inspired by:
- Falling sand simulations
- Cellular automata systems
- Game physics engines

### ⚠️ Limitations

- Not physically accurate (approximation)
- Limited pressure simulation
- CPU performance constraints

### 🔮 Future Improvements

- GPU implementation (OpenCL / CUDA)
- Better pressure handling
- More fluid types (water, lava, etc.)
- Interaction tools

---

Made with ❤️ by Jayson
