### 

    Christopher Celio, 2008 May

###### About

This is my first ever C++ program, written for a class project in
parallelization techniques targeting the Tilera TILE64 many-core processor. 

You can see an example output from my ray tracer here:

[Example scene.](http://www.eecs.berkeley.edu/~celio/Christopher%20Celio_files/by%20default%20235.png)

###### Features

- cosine shading
- specular lighting
- reflections
- runs once to render a static scene
- outputs a .txt file for later, offline rendering

Not supported:

- refraction
- octree parsing
- moving scenes


----------

I make no promises about this code - the quality, the correctness, or even if it compiles anymore. It was originally set up inside Eclipse which auto-generated the build directories and makefiles. Memory leaks abound - coming in as a Java programmer, I was still struggling to learn proper pointer usage, and I didn't yet know about valgrind. Caveat emptor.

----------

Here are some of the links that helped me learn both C++ and raytracing:

    - http://www.devmaster.net/articles/raytracing_series/part2.php


