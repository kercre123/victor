__kernel void hello(
  __global const float* input_arg1, 
  __global const float* input_arg2, 
  __global float* output)
{
  const uint index = get_global_id(0);
  output[index] = input_arg1[index] * input_arg2[index];
}