#include <iostream>
#include <vector>
#include <cassert>

#include <SDL3/SDL.h>
#include <stb_image.h>

#define KERN_RADIUS 8
#define IMG "Lena_512.png"
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

struct Image
{
    int width;
    int height;
    int channels;
    unsigned char* data;
};

auto load_image()
{
    Image result;
    result.data = stbi_load(IMG, &result.width, &result.height, &result.channels, 1); // 1 channels loads grey
    return result;
}

Image mean_filter(const Image& src_img, int window_radius)
{
    auto total_pixels = src_img.width * src_img.height;
    Image result;
    result.data = new unsigned char[total_pixels];
    memset(result.data, 0, total_pixels);

    result.width = src_img.width;
    result.height = src_img.height;
    result.channels = 1;

    // cache of rolling 2d sum
    std::vector<int> sums;
    sums.resize(result.width * result.height);
    // cache of row-wise sums. Should be d * 'n - d' elements
    std::vector<std::vector<int>> row_sums;

    int d = 2 * window_radius;
    int d2 = d * d;

    // initial calc to get rows in the cache
    for (int row = 0; row < d; row++)
    {
        std::vector<int> row_cache;
        int rolling_row_sum = 0;
        int row_idx = row * result.width;
        for (int i = 0; i < d; i++)
        {
            rolling_row_sum += src_img.data[row_idx + i];
        }
        row_cache.push_back(rolling_row_sum);

        for (int col = window_radius + 1; col < result.width - window_radius; col++)
        {
            int prev_sum = row_cache[col - window_radius - 1];
            int next_px = src_img.data[row_idx + col + window_radius];
            int prev_px = src_img.data[row_idx + col - window_radius - 1];
            int new_sum = prev_sum + next_px - prev_px;
            row_cache.push_back(new_sum);
        }
        row_sums.push_back(row_cache);
    }

    // for the first row, we just want to sum what is in cache
    for (int col = window_radius; col < result.width - window_radius; col++)
    {
        int idx = (window_radius * result.width) + col;
        int sum = 0;
        for (int k = 0; k < d; k++)
        {
            sum += row_sums[k][col - window_radius];
        }
        sums[idx] = sum;
    }

    for (int row = window_radius + 1; row < result.height - window_radius; row++)
    {
        // push a new row to the cache and compute the first rowwise sum
        std::vector<int> next_row_cache;
        int rolling_row_sum = 0;
        const auto row_idx = row * result.width;
        const auto last_kern_row_idx = (row + window_radius) * result.width;
        for (int i = 0; i < d; i++)
        {
            rolling_row_sum += src_img.data[last_kern_row_idx + i];
        }
        next_row_cache.push_back(rolling_row_sum);
        row_sums.push_back(next_row_cache);

        for (int col = window_radius; col < result.width - window_radius; col++)
        {
            auto& last_row = row_sums.back();
            auto& first_row = row_sums.front();

            // do this pixel first
            const auto cache_idx = col - window_radius;

            const auto last_avg_idx = ((row - 1) * result.width) + col;
            auto avg = sums[last_avg_idx];
            const auto first_row_sum = first_row[cache_idx];
            const auto last_row_sum = last_row[cache_idx];
            avg += last_row_sum - first_row_sum;
            sums[row_idx + col] = avg;

            if (col + window_radius + 1 < result.width)
            {
                // compute the next row sum
                const auto new_row_val = src_img.data[last_kern_row_idx + col + 1 + window_radius];
                const auto old_row_val = src_img.data[last_kern_row_idx + col + 1 - window_radius];

                const auto new_row_sum = last_row_sum + new_row_val - old_row_val;
                last_row.push_back(new_row_sum);
            }
        }

        row_sums.erase(row_sums.begin());
    }

    // finally we do a pass and just divide everything
    for (int i = 0; i < result.width * result.height; i++)
    {
        const auto s = sums[i];
        result.data[i] = s / d2;
    }

    return result;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("rigel", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    int w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    std::cout << "the screen is " << w << "x" << h << std::endl;

    auto image = load_image();
    if (image.data == nullptr)
    {
        std::cout << "Err, could not laod image" << std::endl;
        return 1;
    }

    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, image.width, image.height);

    unsigned char* tex_data;
    int pitch;
    if (!SDL_LockTexture(texture, nullptr, (void**)&tex_data, &pitch))
    {
        std::cout << "Couldn't write to texture" << std::endl;
        return 1;
    }

    for (int i = 0, out = 0; i < image.width * image.height; i++, out += 3)
    {
        for (int c = 0; c < 3; c++)
        {
            tex_data[out + c] = image.data[i];
        }
    }
    SDL_UnlockTexture(texture);

    auto orig_x = (WINDOW_WIDTH / 2) - image.width;
    auto orig_y = (WINDOW_HEIGHT / 2) - (image.height / 2);
    const SDL_FRect img_rect { (float)orig_x, (float)orig_y, (float)image.width, (float)image.height };

    auto mean_filtered = mean_filter(image, KERN_RADIUS);

    auto mean_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, image.width, image.height);
    if (!SDL_LockTexture(mean_texture, nullptr, (void**)&tex_data, &pitch))
    {
        std::cout << "Couldn't write to mean filteredtexture" << std::endl;
        return 1;
    }

    for (int i = 0, out = 0; i < mean_filtered.width * mean_filtered.height; i++, out += 3)
    {
        for (int c = 0; c < 3; c++)
        {
            tex_data[out + c] = mean_filtered.data[i];
        }
    }
    SDL_UnlockTexture(mean_texture);

    auto mean_x = (WINDOW_WIDTH / 2);
    auto mean_y = (WINDOW_HEIGHT / 2) - (image.height / 2);
    const SDL_FRect mean_img_rect { (float)mean_x, (float)mean_y, (float)mean_filtered.width, (float)mean_filtered.height };
    
    bool should_run = true;
    while (should_run)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_EVENT_QUIT: {
                    should_run = false;
                }
                break;
            }
        }

        SDL_RenderTexture(renderer, texture, nullptr, &img_rect);
        SDL_RenderTexture(renderer, mean_texture, nullptr, &mean_img_rect);
        SDL_RenderPresent(renderer);
    }

    return 0;
}
