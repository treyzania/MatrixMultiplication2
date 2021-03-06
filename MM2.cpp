#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

#define ROWS 3
#define COLS 3

#define BLOCK_SIZE 3
int main()
{
	cl_float *matrix1;
	cl_float *matrix2;
	cl_float *matrix3;
	cl_float *output;
	cl_uint width = COLS;
	cl_uint height = ROWS;

	cl_uint num_devs_returned;
	cl_context_properties properties[3];
	cl_device_id device_id;
	cl_int err;
	cl_platform_id platform_id;
	cl_uint num_platforms_returned;
	cl_context context;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_mem inputBuffer1, inputBuffer2, inputBuffer3, inputBufferTemp, inputBufferTemp2, outputBuffer;
	size_t global[2];
	size_t local[2];

	FILE *fp;
	long filelen;
	long readlen;
	char *kernel_src;

	uint x, y;

	matrix1 = (cl_float*)malloc(sizeof(cl_float)*width*height);
	matrix2 = (cl_float*)malloc(sizeof(cl_float)*width*height);
	matrix3 = (cl_float*)malloc(sizeof(cl_float)*width*height);
	output = (cl_float*)malloc(sizeof(cl_float)*width*height);

	float data1[9] = {
		1, 7, 6,
		3, 5, 8,
		10, 2, 9
	};
	float data2[9] = {
		2, 0, 0,
		0, 2, 0,
		0, 0, 2
	};
	float data3[9] = {
		2, 0, 0,
		4, 0, 0,
		6, 0, 0
	};
	
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			matrix1[y * height + x] = data1[y * height + x];
			matrix2[y * height + x] = data2[y * height + x];
			matrix3[y * height + x] = data3[y * height +x];
			output[y * height + x] = 0;
		}
	}


	fp = fopen("MM2.cl", "r");
	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	rewind(fp);

	kernel_src = (char*)malloc(sizeof(char) * (filelen + 1));
	readlen = fread(kernel_src, 1, filelen, fp);
	if(readlen != filelen)
	{
		printf("Error reading file.\n");
		exit(1);
	}
	kernel_src[filelen + 1] = '\0';

	err = clGetPlatformIDs(1, &platform_id, &num_platforms_returned);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to get Platform ID. Error Code: %d\n", err);
		exit(1);
	}
	
	err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devs_returned);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to get Device ID. Error Code: %d\n", err);
		exit(1);
	}

	properties[0] = CL_CONTEXT_PLATFORM;
	properties[1] = (cl_context_properties) platform_id;
	properties[2] = 0;

	context = clCreateContext(properties, 1, &device_id, NULL, NULL, &err);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to create context. Error Code: %d\n", err);
		exit(1);
	}

	command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to create command queue. Error Code: %d\n", err);
		exit(1);
	}

	program = clCreateProgramWithSource(context, 1, (const char **) &kernel_src, NULL, &err);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to create kernel object. Error Code: %d\n", err);
		exit(1);
	}

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to build code. Error Code: %d\n", err);
		size_t len;
		char buffer[4096];
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("--- Build Log --- \n %s\n", buffer);
		exit(1);
	}

	kernel = clCreateKernel(program, "MatrixMultiplicationTwo", &err);
	if(err != CL_SUCCESS)
	{
		printf("Error: Unable to create kernel object. Error Code: %d\n", err);
		exit(1);
	}

	inputBuffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * ROWS * COLS, matrix1, NULL);
	inputBuffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * ROWS * COLS, matrix2, NULL);
	inputBuffer3 = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * ROWS * COLS, matrix3, NULL);
	inputBufferTemp = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * ROWS * COLS, NULL, NULL);
	inputBufferTemp2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * ROWS * COLS, NULL, NULL);
	outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * ROWS * COLS, NULL, NULL);

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer1);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set first kernel argument. Error Code: %d\n", err);
		exit(1);
	}
	
	err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &inputBuffer2);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set second kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	err =  clSetKernelArg(kernel, 2, sizeof(cl_mem), &inputBuffer3);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set third kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	err =  clSetKernelArg(kernel, 3, sizeof(cl_mem), &inputBufferTemp);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set fourth kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	err =  clSetKernelArg(kernel, 4, sizeof(cl_mem), &inputBufferTemp2);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set fifth kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	err =  clSetKernelArg(kernel, 5, sizeof(cl_mem), &outputBuffer);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set sixth kernel argument. Error Code: %d\n", err);
		exit(1);
	}
	
	err = clSetKernelArg(kernel, 6, sizeof(cl_uint), &width);
	if (err != CL_SUCCESS)
	{
		printf("Unable to set seventh kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	err = clSetKernelArg(kernel, 7, sizeof(cl_uint), &width); 
	if (err != CL_SUCCESS)
	{
		printf("Unable to set eighth kernel argument. Error Code: %d\n", err);
		exit(1);
	}

	global[0] = width;
	global[1] = height;

	local[0] = BLOCK_SIZE;
	local[1] = BLOCK_SIZE;

	err = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global, local, 0, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		printf("Unable to enqueue kernel command. Error Code: %d\n", err);
		exit(1);
	}

	clFinish(command_queue);

	err = clEnqueueReadBuffer(command_queue, outputBuffer, CL_TRUE, 0, sizeof(cl_float) * width * height, output, 0, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		printf("Error enqueuing read buffer command. Error Code: %d\n", err);
		exit(1);
	}

	clReleaseMemObject(inputBuffer1);
	clReleaseMemObject(inputBuffer2);
	clReleaseMemObject(inputBuffer3);
	clReleaseMemObject(inputBufferTemp);
	clReleaseMemObject(inputBufferTemp2);
	clReleaseMemObject(outputBuffer);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
	
	printf("\nMatrix 1\n");
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			printf("%.2f , ", matrix1[y * height + x]);
		}
		printf("\n");
	}

	printf("\nMatrix 2\n");
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			printf("%.2f , ", matrix2[y * height + x]);
		}
		printf("\n");
	}

	printf("\nMatrix 3\n");
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			printf("%.2f , ", matrix3[y * height + x]);
		}
		printf("\n");
	}

	printf("\nOutput Matrix \n");
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			printf("%.2f , ", output[y * height + x]);
		}
		printf("\n");
	}

	free(kernel_src);
	free(matrix1);
	free(matrix2);
	free(matrix3);
	free(output);
	return 0;

}
