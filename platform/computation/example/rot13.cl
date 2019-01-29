__kernel void rot13(__global const char* input, __global char* output)
{
  const uint index = get_global_id(0);
  char c=input[index];
  if (c<'A' || c>'z' || (c>'Z' && c<'a')) {
    output[index] = input[index];
  } else {
    if (c>'m' || (c>'M' && c<'a')) {
      output[index] = input[index]-13;
    } else {
      output[index] = input[index]+13;
    }
  }
}