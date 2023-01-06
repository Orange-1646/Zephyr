# Zephyr
This is my personal take on a vulkan renderer. The whole point of this project is to familiar myself with vulkan and trying out different rendering techniques on it.

This project is by no mean production ready. Code structures are pretty messy, there are a lot of raw pointers floating around, and there is no shader management.

## Current Feature

- Component-based system(A partial take on ECS, without contiguous memory management)
- Graph-based rendering pipeline that supports both graphics and compute pipeline, with resource aliasing and auto synchronization(Based on the framegraph talk by Frostbite)
- Graphics API abstraction layer(currently only vulkan available)
- Full forward and deferred shading with metallic-roughness pbr workflow
- 4 level cascaded shadow mapping with smooth transitioning in between cascades
- Directional light and point light support
- IBL support(SH for diffuse and split-sum approximation for specular)
- Compute based post-processing stack, including:
  - hdr and bloom
  - aces tonemapping
  - fxaa
## Here are some screenshots
- IBL
![IBL Material Screenshot 2023 01 02 - 14 27 58 68](https://user-images.githubusercontent.com/34897676/210200675-c3a4cbe8-9a81-4b46-81ca-038d54a5aa7f.png)
![PBR Model Screenshot 2023 01 02 - 15 26 43 54](https://user-images.githubusercontent.com/34897676/210203845-9f95c3ea-0867-471b-8c5d-19bc262d7353.png)
- hdr && bloom
![Bloom Screenshot 2023 01 06 - 14 11 44 02](https://user-images.githubusercontent.com/34897676/210941384-2cab054a-4edd-460c-bd67-9c0cf87b7242.png)
- cascaded shadow mapping
![Cascaded Shadow Screenshot 2023 01 02 - 15 49 56 54](https://user-images.githubusercontent.com/34897676/210205484-47f43a2c-2833-40b8-ab73-0f8d91e3382a.png)
![Cascaded Shadow Debug Screenshot 2023 01 02 - 15 51 38 59](https://user-images.githubusercontent.com/34897676/210205561-01b588f6-ba0e-450f-b6bc-b966145b0e7b.png)
