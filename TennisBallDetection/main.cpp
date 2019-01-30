#include <stdint.h>

#include <SDL.h>
#include <turbojpeg.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "camera.hpp"

const int DISPLAY_WIDTH = 1280, DISPLAY_HEIGHT = 720;
const int IMAGE_WIDTH = 1280, IMAGE_HEIGHT = 720;

typedef uint8_t* ImageRGB24;
typedef float* ImageRGBFloat;
typedef float* ImageHSVFloat;
typedef uint8_t* ImageGray;

struct rgb {
	double r;       // a fraction between 0 and 1
	double g;       // a fraction between 0 and 1
	double b;       // a fraction between 0 and 1
};

struct hsv {
	double h;       // angle in degrees
	double s;       // a fraction between 0 and 1
	double v;       // a fraction between 0 and 1
};

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);

hsv rgb2hsv(rgb in)
{
	hsv         out;
	double      min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max > in.b ? max : in.b;

	out.v = max;                                // v
	delta = max - min;
	if (delta < 0.00001)
	{
		out.s = 0;
		out.h = 0; // undefined, maybe nan?
		return out;
	}
	if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  // s
	} else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		out.s = 0.0;
		out.h = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = 2.0 + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.h = 4.0 + (in.r - in.g) / delta;  // between magenta & cyan

	out.h *= 60.0;                              // degrees

	if (out.h < 0.0)
		out.h += 360.0;

	return out;
}


rgb hsv2rgb(hsv in)
{
	double      hh, p, q, t, ff;
	long        i;
	rgb         out;

	if (in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}

void image_rgb_to_hsv(ImageRGB24 image, ImageHSVFloat res) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		float r = (float)image[i * 3 + 0] / 255.0f;
		float g = (float)image[i * 3 + 1] / 255.0f;
		float b = (float)image[i * 3 + 2] / 255.0f;

		rgb color = { r, g, b };
		hsv color_out = rgb2hsv(color);

		res[i * 3 + 0] = color_out.h;
		res[i * 3 + 1] = color_out.s;
		res[i * 3 + 2] = color_out.v;
	}
}

// Produces a greyscale image from an HSV image.
void filter_image(ImageHSVFloat pixels, float min_hue, float max_hue, float min_sat, float max_sat, ImageGray out) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		float h = pixels[i * 3 + 0];
		float s = pixels[i * 3 + 1];
		float v = pixels[i * 3 + 2];

		if (h >= min_hue && h <= max_hue && s >= min_sat && s <= max_sat) { // color_out.h >= 70.0f && color_out.h <= 110.0f
			uint8_t intensity = (uint8_t)(255.0f * v);

			out[i] = intensity;
		} else {
			out[i] = 0;
		}

		if (v < 0.5f) {
			out[i] = 0;
		}
	}
}

void blur_image(ImageGray pixels, int kernel_size) {
	for (int y = 0; y < IMAGE_HEIGHT; y++) {
		for (int x = 0; x < IMAGE_WIDTH; x++) {
			int sum = 0;

			for (int ky = -(kernel_size / 2); ky <= kernel_size/2; ky++) {
				for (int kx = -(kernel_size / 2); kx <= kernel_size/2; kx++) {
					int nx = x + kx;
					int ny = y + ky;

					if (nx < 0 || nx >= IMAGE_WIDTH || ny < 0 || ny >= IMAGE_HEIGHT) {
						// Update sum with 0, so do nothing.
					} else {
						sum += pixels[ny * IMAGE_WIDTH + nx];
					}
				}
			}

			pixels[y * IMAGE_WIDTH + x] = (uint8_t)(sum / (kernel_size*kernel_size));
		}
	}
}

int value_at(int* input, int width, int height, int x, int y) {
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return 0;
	}

	return input[y * width + x];
}

// Does a 3x3 convolution.
int* convolution3(int* input, int width, int height, int* kernel) {
	int* output = new int[width * height];

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int sum = 0;

			for (int ky = 0; ky < 3; ky++) {
				for (int kx = 0; kx < 3; kx++) {
					int v = value_at(input, width, height, x + kx - 1, y + ky - 1);
					sum += v * kernel[ky * 3 + kx];
				}
			}

			output[y * width + x] = sum;
		}
	}

	return output;
}

void threshold(ImageGray pixels, float threshold) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		uint8_t val = pixels[i];

		if ((float)val > threshold * 255.0f) {
			pixels[i] = 255;
		} else {
			pixels[i] = 0;
		}
	}
}

void edge_detection(ImageGray pixels, float threshold) {
	uint8_t* out = new uint8_t[IMAGE_WIDTH * IMAGE_HEIGHT];

	for (int y = 0; y < IMAGE_HEIGHT; y++) {
		for (int x = 0; x < IMAGE_WIDTH; x++) {
			uint8_t val = pixels[y * IMAGE_WIDTH + x];

			if ((float)val > threshold * 255.0f) {
				bool found_edge = false;

				for (int ky = -1; ky <= 1; ky++) {
					for (int kx = -1; kx <= 1; kx++) {
						int nx = x + kx;
						int ny = y + ky;

						uint8_t nval;

						if (nx < 0 || nx >= IMAGE_WIDTH || ny < 0 || ny >= IMAGE_HEIGHT) {
							nval = 0;
						} else {
							nval = pixels[ny * IMAGE_WIDTH + nx];
						}

						if ((float)nval <= threshold * 255.0f) {
							// This neighbor makes us an edge.

							found_edge = true;
						}
					}
				}

				if (found_edge) {
					out[y * IMAGE_WIDTH + x] = 255;
				} else {
					out[y * IMAGE_WIDTH + x] = 0;
				}
			} else {
			out[y * IMAGE_WIDTH + x] = 0;
			}
		}
	}

	memcpy(pixels, out, IMAGE_WIDTH * IMAGE_HEIGHT);

	delete[] out;
}

void update_texture_grey(SDL_Texture* texture, ImageGray image_data) {
	uint8_t* pixels;
	int pitch;

	SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		pixels[i * 3 + 0] = image_data[i];
		pixels[i * 3 + 1] = image_data[i];
		pixels[i * 3 + 2] = image_data[i];
	}

	SDL_UnlockTexture(texture);
}

void update_texture_hsv(SDL_Texture* texture, ImageHSVFloat image_data) {
	uint8_t* pixels;
	int pitch;

	SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		hsv color = { image_data[i * 3 + 0], image_data[i * 3 + 1], image_data[i * 3 + 2] };
		rgb color_out = hsv2rgb(color);

		pixels[i * 3 + 0] = (uint8_t)(255.0f * color_out.r);
		pixels[i * 3 + 1] = (uint8_t)(255.0f * color_out.g);
		pixels[i * 3 + 2] = (uint8_t)(255.0f * color_out.b);
	}

	SDL_UnlockTexture(texture);
}

void update_texture_rgb(SDL_Texture* texture, ImageRGB24 image_data) {
	uint8_t* pixels;
	int pitch;

	SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

	memcpy(pixels, image_data, IMAGE_WIDTH * IMAGE_HEIGHT * 3);

	SDL_UnlockTexture(texture);
}

// HSV
// Cumulative moving average (https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average)
float average_hue(ImageHSVFloat pixels) {
	float avg = 0;

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		avg = (pixels[i * 3 + 0] + i * avg) / (i + 1);
	}

	return avg;
}

// HSV
void adjust_average_hue(ImageHSVFloat pixels, float desired_average) {
	// If s is the sum of all pixel hues...
	// ... and n is the pixel count, and a is the average hue...
	// We have s/n = a.
	// We then adjust the average, so we have ks/n = a' => ka = a'.
	// We can solve for k... k = a'/a.
	// We then distribute k over the sum s by multiplying it by each pixel hue.

	float ah = average_hue(pixels);

	float k = desired_average / ah;

	printf("k = %f\n", k);

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		pixels[i * 3 + 0] *= k;

		while (pixels[i * 3 + 0] > 360.0f) {
			pixels[i * 3 + 0] -= 360.0f;
		}
	}
}

// HSV
void adjust_average_hue_addition(ImageHSVFloat pixels, float desired_average) {
	// This one does addition.
	// s/n = a => s/n + q = a' => distribute q over each one.

	float ah = average_hue(pixels);

	float q = desired_average - ah;

	printf("q = %f\n", q);

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		pixels[i * 3 + 0] += q;

		if (pixels[i * 3 + 0] > 360.0f) {
			pixels[i * 3 + 0] -= 360.0f;
		}
	}
}

// HSV0
void fix_sat_val(ImageHSVFloat pixels) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		//pixels[i * 3 + 1] = 1.0f;
		pixels[i * 3 + 2] = 1.0f;
	}
}

// RGB 
void normalize_rgb(ImageRGB24 pixels) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		uint8_t r = pixels[i * 3 + 0];
		uint8_t g = pixels[i * 3 + 1];
		uint8_t b = pixels[i * 3 + 2];

		float r_rat = (float)r / (float)(r + g + b);
		float g_rat = (float)g / (float)(r + g + b);
		float b_rat = (float)b / (float)(r + g + b);

		pixels[i * 3 + 0] = (uint8_t)(r_rat * 255.0f);
		pixels[i * 3 + 1] = (uint8_t)(g_rat * 255.0f);
		pixels[i * 3 + 2] = (uint8_t)(b_rat * 255.0f);
	}
}

void gray_world_rgb(ImageRGBFloat pixels) {
	float r_sum = 0;
	float g_sum = 0;
	float b_sum = 0;

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		r_sum += pixels[i * 3 + 0];
		g_sum += pixels[i * 3 + 1];
		b_sum += pixels[i * 3 + 2];
	}

	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		float r = (IMAGE_WIDTH * IMAGE_HEIGHT * pixels[i * 3 + 0]) / r_sum;
		float g = (IMAGE_WIDTH * IMAGE_HEIGHT * pixels[i * 3 + 1]) / g_sum;
		float b = (IMAGE_WIDTH * IMAGE_HEIGHT * pixels[i * 3 + 2]) / b_sum;

		pixels[i * 3 + 0] = r;
		pixels[i * 3 + 1] = g;
		pixels[i * 3 + 2] = b;
	}
}

void image_rgb_to_float_rgb(ImageRGB24 pixels, ImageRGBFloat out) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		out[i * 3 + 0] = pixels[i * 3 + 0] / 255.0f;
		out[i * 3 + 1] = pixels[i * 3 + 1] / 255.0f;
		out[i * 3 + 2] = pixels[i * 3 + 2] / 255.0f;
	}
}

void float_rgb_to_hsv(ImageRGBFloat image, ImageHSVFloat res) {
	for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
		float r = image[i * 3 + 0];
		float g = image[i * 3 + 1];
		float b = image[i * 3 + 2];

		rgb color = { r, g, b };
		hsv color_out = rgb2hsv(color);

		res[i * 3 + 0] = color_out.h;
		res[i * 3 + 1] = color_out.s;
		res[i * 3 + 2] = color_out.v;
	}
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window* window = SDL_CreateWindow("Tennis Ball Detection", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, IMAGE_WIDTH, IMAGE_HEIGHT);

	//int x;
	//int y;
	//int channels;
	//ImageRGB24 rgb_pixels = (ImageRGB24) stbi_load("urc_2017/0_9.jpg", &x, &y, &channels, 3); // C:\\Users\\Layne\\Pictures\\Tennis Ball Images\\p1_e0_cropped.jpg
	
	ImageRGB24 rgb_pixels = new uint8_t[IMAGE_WIDTH * IMAGE_HEIGHT * 3];
	ImageRGBFloat rgb_float_pixels = new float[IMAGE_WIDTH * IMAGE_HEIGHT * 3];
	ImageHSVFloat hsv_pixels = new float[IMAGE_WIDTH * IMAGE_HEIGHT * 3];
	ImageGray gray_pixels = new uint8_t[IMAGE_WIDTH * IMAGE_HEIGHT];


	camera::CaptureSession session;
	if (camera::open(&session, "/dev/video0", IMAGE_WIDTH, IMAGE_HEIGHT) != camera::Error::OK) {
		fprintf(stderr, "[!] Failed to open camera!\n");
		return 1;
	}

	if (camera::start(&session) != camera::Error::OK) {
		fprintf(stderr, "[!] Failed to start camera session!\n");
		return 1;
	}

	tjhandle decompressor = tjInitDecompress();

	bool show_gray = false;

	while (true) {
		bool should_quit = false;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				should_quit = true;
				break;
			}

			if (event.type == SDL_MOUSEMOTION) {
				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);

				float h = hsv_pixels[(mouse_y * IMAGE_WIDTH + mouse_x) * 3 + 0];
				float s = hsv_pixels[(mouse_y * IMAGE_WIDTH + mouse_x) * 3 + 1];
				float v = hsv_pixels[(mouse_y * IMAGE_WIDTH + mouse_x) * 3 + 2];

				printf("> Current Pixel HSV: (%f, %f, %f)\n", h, s, v);
			}

			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_1) {
					show_gray = false;
				} else if (event.key.keysym.sym == SDLK_2) {
					show_gray = true;
				}
			}
		}

		if (should_quit) break;

		uint8_t* frame;
		size_t size;
		if (camera::grab_frame(&session, &frame, &size) != camera::Error::OK) {
			fprintf(stderr, "[!] Failed to grab frame!\n");
			return 1;
		}

		tjDecompress2(decompressor, frame, size, rgb_pixels, IMAGE_WIDTH, IMAGE_WIDTH * 3, IMAGE_HEIGHT, TJPF_RGB, 0);

		camera::return_buffer(&session);

		{
			image_rgb_to_float_rgb(rgb_pixels, rgb_float_pixels);

			float_rgb_to_hsv(rgb_float_pixels, hsv_pixels);

			update_texture_hsv(texture, hsv_pixels);

			filter_image(hsv_pixels, 40.0f, 100.0f, 0.4f, 1.0f, gray_pixels);

			// Now we convert to grayscale.

			for (int i = 0; i < 8; i++) {
				blur_image(gray_pixels, 5);
			}

			threshold(gray_pixels, 0.2f);

			edge_detection(gray_pixels, 0.4f);

			if (show_gray) {
				update_texture_grey(texture, gray_pixels);
			} else {
				update_texture_hsv(texture, hsv_pixels);
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_RenderCopy(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);
	}

	stbi_image_free(rgb_pixels);

	return 0;
}