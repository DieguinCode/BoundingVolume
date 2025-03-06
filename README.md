# BoundingVolume


## How to Execute

To successfully execute this project, please follow the steps below:

1. **Ensure you are using Ubuntu**

   This project has been developed and tested on the Ubuntu operating system. Compatibility with other operating systems is not guaranteed.

2. **Install GLFW**

   Open your terminal and run the following commands:

   ```bash
   sudo apt-get install libglfw3
   sudo apt-get install libglfw3-dev
   ```

3. **Install GLM**

   Open your terminal and run the following command:

   ```bash
   sudo apt install libglm-dev
   ```

4. **Navigate to the Project Repository**

   Change the current directory to the root of the project repository:

   ```bash
   cd /path/to/project/repository
   ```

5. **Create a Bin Paste**

   Create a paste to save the executable

   ```bash
   mkdir Bin
   ```

6. **Compile the Project**

   Use the `make` command to compile the project:

   ```bash
   make compile
   ```

7. **Run the Project**

   After compiling, execute the project with the following command:

   ```bash
   make run
   ```

## Manual

- **Press R**: Randomly generates points in the cloud.
- **Press E**: Clear everything.
- **Press A**: Calculate AABB.
- **Press C**: Calculate Circle.
- **Press O**: Calculate OBB.

- **Mouse Click Left**: Create points and check if these points belong or not to the Bouding Volume.

## Explanations
- **White Points**: Detect collision between Bounding Volumes

## Exhibition

SOON // TODO