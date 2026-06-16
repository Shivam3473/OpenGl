// Advanced Data

// A buffer in OpenGL is, at its core, an object that manages a certain piece of GPU memory and nothing more. We give meaning to a buffer when binding it to a specific buffer target. A buffer is only a vertex array buffer when we bind it to GL_ARRAY_BUFFER, but we could just as easily bind it to GL_ELEMENT_ARRAY_BUFFER. OpenGL internally stores a reference to the buffer per target and, based on the target, processes the buffer differently.

//basically when we talk about buffer data and use the function we assign the memory and fill the gpu data in it , howeverr if we use null we could simply reserve the memory and not fill the data , later we could come back to that buffer to fill the data into it

// glBufferSubData(GL_ARRAY_BUFFER, 24, sizeof(data), &data); // Range: [24, 24 + sizeof(data)] , so with this we can only fill specific parts of the buffer , there is this offset and then pointer to the data , we with this just fills the data in it

// simialrly at other times we can use glmapbuffer and this would simply provide us the pointer to the memory to the buffer and we could pass in the data to that pitner 
/*
float data[] = {
  0.5f, 1.0f, -0.35f
  [...]
};
glBindBuffer(GL_ARRAY_BUFFER, buffer);
// get pointer
void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
// now copy data into memory
memcpy(ptr, data, sizeof(data));
// make sure to tell OpenGL we're done with the pointer
glUnmapBuffer(GL_ARRAY_BUFFER);
 */



 // BATCHING VERTEX ATTRIBUTES ---> next it is important to know about the working of the vertex atrib pointer, basically, until now we placed both the positions and the texture coordiantes at one single array but now we can place them in diff arrays and use glBufferSubData function , think of it as earlier we filled thememory as 123123123 but now we will fill it as 111222333 
 /*
 float positions[] = { ... };
float normals[] = { ... };
float tex[] = { ... };
// fill buffer
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions);
glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(normals), &normals);
glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(normals), sizeof(tex), &tex);

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);  
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(positions)));  
glVertexAttribPointer(
  2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(positions) + sizeof(normals)));  
 */