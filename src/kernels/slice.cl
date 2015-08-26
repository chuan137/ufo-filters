kernel void
copy (global float *input,
      global float *output,
      private int offset)
{
    int idx = get_global_id (1) * get_global_size (0) + get_global_id (0);
    output[idx] = (float) input[offset + idx];
}


