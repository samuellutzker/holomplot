
Holomplot v1.0 (GL)
===================

A program for plotting expressions in holomorphic functions using OpenGL.

Libraries
---------
- wxWidgets 3.2.6  (GUI framework)
- GLEW 2.2.0       (OpenGL Extension Wrangler)
- GLM 0.9.9        (Mathematics library for OpenGL)
- TBB 2022.0.0     (Parallel processing)

Usage
-----
- Enter an expression in the provided input field.
- Enter desired accuracy / resolution.
- Adjust camera position using mouse dragging and wheel.

Example Expressions
-------------------
1. atan(-10 + x^2 + y^2 / 5)
2. 2sqrt(max(0,1-x^2/64-y^2/64))cos(sqrt(x^2+y^2))
3. sin(ln(exp(z)))
4. (sin(x^2 - y^2)) / (1 + sqrt(x^2 + y^2))
5. sqrt(max(0,1-(sqrt(x^2+y^2)-2)^2))
6. z^7exp(-abs(z)^2)

![holomplot](https://github.com/user-attachments/assets/fce85c24-c9e1-4c46-ab25-434fc3c2e556)
