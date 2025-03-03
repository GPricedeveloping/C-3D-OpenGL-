Project Overview
The project was to create a completed 3D scene from an everyday reference. The scene resembles a workspace composition from my Japanese desk, including main objects such as a table, laptop, panda tumbler, mouse, window, couch, and walls. The objective was to model the objects correctly and apply realistic lighting, textures, and camera control to achieve improved visual interest and immersion.

Development Process
Designing Software
•	New Skills Acquired in Design:
o	This project developed my ability to break down actual-world environments into fundamental 3D shapes and employing suitable lighting and texturing techniques for realism.
•	Design Process of the Project:
o	The process employed an iterative technique, starting from object modeling, followed by lighting fine-tuning, texture mapping, and adjusting camera settings.
•	Future Application of Design Strategies:
o	The systematic 3D modeling and rendering procedure will be useful for application in game development, simulations, and interactive media where realism is paramount in the environment.

Creating Programs
•	New Development Methods Utilized:
o	Used applied OpenGL transformations to correctly scale, position, and align objects.
o	Adjusted Phong shading calculations for natural lighting effects.
o	Used custom functions to maintain modularity and readability.
•	Iteration in Development:
o	Iterative testing was crucial for lighting balance, ensuring proper shadow placement and reflections.
o	Texture mapping adjustments were made to prevent distortions in UV scaling.
•	Code Evolution Throughout Milestones:
o	Initial versions had basic lighting and object placement.
o	Later iterations incorporated finer control over shaders, light intensities, and camera movement for a more polished experience.

Applying Computer Science to My Goals
•	Impact of Computational Graphics and Visualizations:
o	The project has provided hands-on exposure to graphics programming principles such as shaders, transformations, and lighting models.
o	Understanding 3D rendering techniques will be useful in future projects involving game engines (Unreal Engine, Godot) or real-time visual simulations.
•	Future Professional and Educational Applications:
o	The techniques used here will be used in data visualization careers that need data visualizing, UI/UX designing, and interactive programs.

Project Features
•	3D Modeled Objects:
o	All objects were hand-modeled to look like the scene they were based on.
•	Advanced Lighting Setup:
o	Window Light: Mimics real daylight into the scene.
o	Ceiling Light: Provides ambient lighting.
o	Filler Lights: Reduces dark areas for visibility.
•	User Navigation Controls:
o	WASD Keys: Maps to movement in directions that are horizontal.
o	Q/E Keys: Move up and down.
o	Mouse Movement: Controls the camera angle.
o	Scroll Wheel: Alters movement speed for fine control.
•	Optimized Code Structure:
o	Modular functions (SetTransformations(), SetShaderMaterial(), SetupSceneLights()) enhance readability and maintainability.

Challenges & Solutions
•	Lighting and Shadows
o	Problem: Early lighting was too dark, and objects were hard to discern.
o	Solution: Altered ambient, diffuse, and specular lighting and adjusted Phong shading for realistic lighting.
•	Texture Mapping
o	Problem: Textures were distorted or misaligned because of improper UV scaling.
o	Solution: Recalculated UV mapping coordinates to accurately project texture.

Future Improvements
•	Shadow Mapping: Adding real-time shadows for enhanced realism.
•	Reflections: Inserting real-time reflections on glass and metal surfaces.
•	Performance Optimization: Reducing computation overhead in lighting calculations.
•	Bump Mapping: Adding texture depth to more closely replicate surface detail.

Repository Contents
•	3D Scene ZIP Folder: All project files, object models, textures, and shaders.
•	Design Decisions Document: Describes the development decisions made.
•	README.md: This file, describing project information and main points.

