#include <iostream>
#include <vector>

#include <SDL3/SDL.h>
#include <stb_image.h>

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
    result.data = stbi_load("Lena_512.png", &result.width, &result.height, &result.channels, 1); // 1 channels loads grey
    return result;
}

Image mean_filter(const Image& src_img, int window_radius)
{
    auto total_pixels = src_img.width * src_img.height;
    Image result;
    result.data = new unsigned char[total_pixels];

    // zero it
    memset(result.data, 0, total_pixels);

    result.width = src_img.width;
    result.height = src_img.height;
    result.channels = 1;

    // should be `d` rows of `n - d` rolling sums
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
            int prev_px = src_img.data[row_idx + col - window_radius - 1];
            int next_px = src_img.data[row_idx + col + window_radius];
            int new_sum = prev_sum + next_px - prev_px;
            row_cache.push_back(new_sum);
        }
        row_sums.push_back(row_cache);
    }

    // we now have a full row sum cache. it has the 1d sum along the row centered at each px in it.
    for (int row = window_radius; row < result.height - window_radius; row++)
    {
        // get the rolling average value for this pixel by summing along the column in the cache
        int row_idx = row * result.width;
        for (int col = window_radius; col < result.width - window_radius; col++)
        {
            int avg = 0;
            int cache_idx = col - window_radius;
            for (int row_cache = 0; row_cache < d; row_cache++)
            {
                avg += row_sums[row_cache][cache_idx];
            }
            result.data[row_idx + col] = avg / d2;
            
            // get the next col value
            if (col < result.width - window_radius - 1)
            {
                auto last_row_idx = (row + window_radius) * result.width;

                int new_val = src_img.data[last_row_idx + col + window_radius + 1];
                int old_val = src_img.data[last_row_idx + col - window_radius];

                auto& last_row = row_sums.back();
                auto new_sum = last_row[cache_idx] + new_val - old_val;
                last_row.push_back(new_sum);
            }
        }

        if (row < result.height - window_radius - 1)
        {
            // cycle the row cache
            row_sums.erase(row_sums.begin());
            std::vector<int> next_row_cache;
            int rolling_row_sum = 0;
            int row_idx = (row + window_radius + 1) * result.width;
            for (int i = 0; i < d; i++)
            {
                rolling_row_sum += src_img.data[row_idx + i];
            }
            next_row_cache.push_back(rolling_row_sum);
            row_sums.push_back(next_row_cache);
        }
    }

    return result;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("rigel", 1920, 1080, 0);
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

    auto orig_x = (1920 / 2) - image.width;
    auto orig_y = (1080 / 2) - (image.height / 2);
    const SDL_FRect img_rect { (float)orig_x, (float)orig_y, (float)image.width, (float)image.height };

    auto mean_filtered = mean_filter(image, 4);
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

    auto mean_x = (1920 / 2);
    auto mean_y = (1080 / 2) - (image.height / 2);
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
