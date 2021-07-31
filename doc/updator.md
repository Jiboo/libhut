An updator allows to avoid an allocation when updating an image/buffer by giving direct access to a memory mapped
staging buffer allocation.

Upon call to prepare_update(..), a staging area is allocated on which you can work during the whole life of the updator
object, when it is destructed, it will schedule the update from the staging zone, and free the staging memory when done.

The alternative would be for you to allocate a buffer, work on it, and give it to hut, which will copy it in a staging
area, so with the update you directly get a buffer to the staging area.

    struct bitmap_from_somelib {
        uint width, height, pitch;
        uint8_t *pixels;
    };

    // Some bitmap you want to upload to the GPU
    bitmap_from_somelib mybmp;

    // Allocate the hut image which will receive the data
    image_params img_params{.size = {mybmp.width, mybmp.height}, .format = VK_FORMAT_R8_UNORM};
    shared_image dst = std::make_shared<image>(_display, img_params);

    // Use an updator to alloc a staging buffer big enough to update the whole image
    image::updator update = dst->prepare_update();

    // Directly work on the staging area
    uint row_byte_size = dst->pixel_size() * mybmp.width;
    for (uint y = 0; y < _params.size_.y; y++) {
        uint8_t *dst_row = update.data() + y * update.staging_row_pitch();
        uint8_t *src_row = mybmp.pixels + y * mybmp.pitch;
        memcpy(dst_row, src_row, row_byte_size);
    }

    // Upon leaving the scope, the updator is destructed, the copy to the image is scheduled,
    // and the staging area will be freed after the copy is done on the GPU.
