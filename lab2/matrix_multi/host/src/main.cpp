/*
EECE6540 - Heterogeneous Computing
Lab 2 - Host Source File
J.M. Gallagher
27-Jun-2021

Adapted from materials provided by Yan Luo
at https://github.com/ACANETS/eece-6540-labs/tree/master/Labs/lab2
File main.cpp
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

#define STRING_BUFFER_LEN 1024

// Runtime constants
// Used to define the work set over which this kernel will execute.
static const size_t work_group_size = 8;  // 8 threads in the demo workgroup
// Defines kernel argument value, which is the workitem ID that will
// execute a printf call
static const int thread_id_to_output = 2;

// OpenCL runtime configuration
static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue queue = NULL;
static cl_kernel kernel = NULL;
static cl_program program = NULL;

// Control whether the emulator should be used.
static bool use_emulator = false;

// Function prototypes
bool init();
void cleanup();
static void device_info_ulong( cl_device_id device, cl_device_info param, const char* name);
static void device_info_uint( cl_device_id device, cl_device_info param, const char* name);
static void device_info_bool( cl_device_id device, cl_device_info param, const char* name);
static void device_info_string( cl_device_id device, cl_device_info param, const char* name);
static void display_device_info( cl_device_id device );

// Entry point.
int main(int argc, char** argv) {

  Options options(argc, argv);

  // Program expects command line arguments m, n, p to set matrix sizes
  // The program is not designed to perform any validation on the input values
  // Defaults: m = 200, n = 400, p = 600 

  int ret;

  // declare matrix sizes
  size_t wA = 400;
  size_t hA = 200;
  size_t wB = 600;

  // Optional argument to specify the problem size.
  if(options.has("m")) {
    hA = options.get<unsigned>("m");
  }
  // Optional argument to specify the problem size.
  if(options.has("n")) {
    wA = options.get<unsigned>("n");
  }
  // Optional argument to specify the problem size.
  if(options.has("p")) {
    wB = options.get<unsigned>("p");
  }

  size_t hB = wA;
  size_t wC = wB; // wD = wC
  size_t hC = hA; // hD = hA

  float A[wA*hA]; 
  float B[wB*hB];
  float C[wC*hC];

  for(int i = 0; i < hA*wA; i++) {
    A[i] = 1.0f;
  }

  for(int i = 0; i < hB*wB; i++) {
    B[i] = 2.0f;
  }

  for(int i = 0; i < hC*wC; i++) {
    C[i] = 3.0f;
  }

  // Optional argument to specify whether the emulator should be used.
  if(options.has("emulator")) {
    use_emulator = options.get<bool>("emulator");
  }

  cl_int status;

  if(!init()) {
    return -1;
  }

  float *D = (float *)calloc (hC * wC ,  sizeof(float));
  /* Code to print output matrix to show it is filled with 0s
  for (int i = 0; i < wC*hC; i++) {
    printf ("%f ", D[i]);
  }
  printf("\n");
  */

  // In this example, we assume A, B, C are float arrays which
  // have been declared and initialized
 
  // allocate space for Matrix A on the device 
  cl_mem bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY,
          wA*hA*sizeof(float), NULL, &ret);
  // copy Matrix A to the device 
  clEnqueueWriteBuffer(queue, bufferA, CL_TRUE, 0,
          wA*hA*sizeof(float), (void *) A, 0, NULL, NULL);

  // allocate space for Matrix B on the device 
  cl_mem bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY,
          wB*hB*sizeof(float), NULL, &ret);
  // copy Matrix B to the device 
  clEnqueueWriteBuffer(queue, bufferB, CL_TRUE, 0,
          wB*hB*sizeof(float), (void *) B, 0, NULL, NULL);

  // allocate space for Matrix C on the device 
  cl_mem bufferC = clCreateBuffer(context, CL_MEM_READ_ONLY,
          wC*hC*sizeof(float), NULL, &ret);
  // copy Matrix C to the device 
  clEnqueueWriteBuffer(queue, bufferC, CL_TRUE, 0,
          wC*hC*sizeof(float), (void *) C, 0, NULL, NULL);

  // allocate space for Matrix D on the device 
  cl_mem bufferD = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
          wC*hC*sizeof(float), NULL, &ret);

  // Set the kernel arguments
  status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&bufferD);
  status = clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&wA);
  status = clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&hA);
  status = clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&wB);
  status = clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&hB);
  status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&bufferA);
  status = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&bufferB);
  status = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&bufferC);

  checkError(status, "Failed to set kernel arg ");

  printf("\nKernel initialization is complete.\n");
  printf("Launching the kernel...\n\n");

  // Configure work set over which the kernel will execute
  size_t globalws[2]={wC, hC};
  size_t localws[2] = {2, 2};
  // Execute the kernel 
  status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL,
      globalws, localws, 0, NULL, NULL);
  // it is important to check the return value.
  checkError(status, "Failed to launch kernel");

  // Wait for command queue to complete pending events
  status = clFinish(queue);
  checkError(status, "Failed to finish");

  printf("\nKernel execution is complete.\n");

  // Copy the output data back to the host 
  clEnqueueReadBuffer(queue, bufferD, CL_TRUE, 0, wC*hC*sizeof(float),
         (void *)D, 0, NULL, NULL);

  // Print the first row of the output matrix
  for (int i = 0; i < wC; i++) {
    printf ("%f ", D[i]);
  }
  printf("\n");

  // Free the resources allocated
  cleanup();
  free(D);

  return 0;
}

/////// HELPER FUNCTIONS ///////

bool init() {
  cl_int status;

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  if (use_emulator) {
    platform = findPlatform("Intel(R) FPGA Emulation Platform for OpenCL(TM)");
  } else {
    platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");
  }
  if(platform == NULL) {
    printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
    return false;
  }

  // User-visible output - Platform information
  {
    char char_buffer[STRING_BUFFER_LEN]; 
    printf("Querying platform for info:\n");
    printf("==========================\n");
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, STRING_BUFFER_LEN, char_buffer, NULL);
    printf("%-40s = %s\n", "CL_PLATFORM_NAME", char_buffer);
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, STRING_BUFFER_LEN, char_buffer, NULL);
    printf("%-40s = %s\n", "CL_PLATFORM_VENDOR ", char_buffer);
    clGetPlatformInfo(platform, CL_PLATFORM_VERSION, STRING_BUFFER_LEN, char_buffer, NULL);
    printf("%-40s = %s\n\n", "CL_PLATFORM_VERSION ", char_buffer);
  }

  // Query the available OpenCL devices.
  scoped_array<cl_device_id> devices;
  cl_uint num_devices;

  devices.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));

  // We'll just use the first device.
  device = devices[0];

  // Display some device information.
  display_device_info(device);

  // Create the context.
  context = clCreateContext(NULL, 1, &device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the command queue.
  queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");

  // Create the program.
  std::string binary_file = getBoardBinaryFile("matrix_multi", device);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create the kernel - name passed in here must match kernel name in the
  // original CL file, that was compiled into an AOCX file using the AOC tool
  const char *kernel_name = "simpleMultiply";  // Kernel name, as defined in the CL file
  kernel = clCreateKernel(program, kernel_name, &status);
  checkError(status, "Failed to create kernel");

  return true;
}

// Free the resources allocated during initialization
void cleanup() {
  if(kernel) {
    clReleaseKernel(kernel);  
  }
  if(program) {
    clReleaseProgram(program);
  }
  if(queue) {
    clReleaseCommandQueue(queue);
  }
  if(context) {
    clReleaseContext(context);
  }
}

// Helper functions to display parameters returned by OpenCL queries
static void device_info_ulong( cl_device_id device, cl_device_info param, const char* name) {
   cl_ulong a;
   clGetDeviceInfo(device, param, sizeof(cl_ulong), &a, NULL);
   printf("%-40s = %lu\n", name, a);
}
static void device_info_uint( cl_device_id device, cl_device_info param, const char* name) {
   cl_uint a;
   clGetDeviceInfo(device, param, sizeof(cl_uint), &a, NULL);
   printf("%-40s = %u\n", name, a);
}
static void device_info_bool( cl_device_id device, cl_device_info param, const char* name) {
   cl_bool a;
   clGetDeviceInfo(device, param, sizeof(cl_bool), &a, NULL);
   printf("%-40s = %s\n", name, (a?"true":"false"));
}
static void device_info_string( cl_device_id device, cl_device_info param, const char* name) {
   char a[STRING_BUFFER_LEN]; 
   clGetDeviceInfo(device, param, STRING_BUFFER_LEN, &a, NULL);
   printf("%-40s = %s\n", name, a);
}

// Query and display OpenCL information on device and runtime environment
static void display_device_info( cl_device_id device ) {

   printf("Querying device for info:\n");
   printf("========================\n");
   device_info_string(device, CL_DEVICE_NAME, "CL_DEVICE_NAME");
   device_info_string(device, CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR");
   device_info_uint(device, CL_DEVICE_VENDOR_ID, "CL_DEVICE_VENDOR_ID");
   device_info_string(device, CL_DEVICE_VERSION, "CL_DEVICE_VERSION");
   device_info_string(device, CL_DRIVER_VERSION, "CL_DRIVER_VERSION");
   device_info_uint(device, CL_DEVICE_ADDRESS_BITS, "CL_DEVICE_ADDRESS_BITS");
   device_info_bool(device, CL_DEVICE_AVAILABLE, "CL_DEVICE_AVAILABLE");
   device_info_bool(device, CL_DEVICE_ENDIAN_LITTLE, "CL_DEVICE_ENDIAN_LITTLE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_SIZE, "CL_DEVICE_GLOBAL_MEM_SIZE");
   device_info_bool(device, CL_DEVICE_IMAGE_SUPPORT, "CL_DEVICE_IMAGE_SUPPORT");
   device_info_ulong(device, CL_DEVICE_LOCAL_MEM_SIZE, "CL_DEVICE_LOCAL_MEM_SIZE");
   device_info_ulong(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, "CL_DEVICE_MAX_CLOCK_FREQUENCY");
   device_info_ulong(device, CL_DEVICE_MAX_COMPUTE_UNITS, "CL_DEVICE_MAX_COMPUTE_UNITS");
   device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_ARGS, "CL_DEVICE_MAX_CONSTANT_ARGS");
   device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE");
   device_info_uint(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
   device_info_uint(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, "CL_DEVICE_MEM_BASE_ADDR_ALIGN");
   device_info_uint(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE");

   {
      cl_command_queue_properties ccp;
      clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &ccp, NULL);
      printf("%-40s = %s\n", "Command queue out of order? ", ((ccp & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)?"true":"false"));
      printf("%-40s = %s\n", "Command queue profiling enabled? ", ((ccp & CL_QUEUE_PROFILING_ENABLE)?"true":"false"));
   }
}

