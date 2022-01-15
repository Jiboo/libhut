An updator allows to avoid an allocation when updating an image/buffer by giving direct access to a memory mapped
staging buffer allocation.

Upon call to update(..), a staging area is allocated on which you can work during the whole life of the updator object,
when it is destructed, it will schedule the update from staging to VRAM, and free the staging when the copy is done.
Updators can be instantiated/destructed from threads safely.

The alternative would be for you to allocate a RAM buffer, work on it, and pass it to hut (like image::update that takes
a span), which hut will copy again in the staging area, so by using the updator directly we do one less copy.

Please note that for image updators, you must respect the pitch (row byte size) of the staging zone, which needs to be
aligned to a hardware dependent preferred value.

    struct bitmapref_from_somelib {
        uint width, height, pitch;
        uint8_t *pixels;
    };

    // Allocate the hut image which will receive the data
    image_params img_params{.size = {mybmp.width, mybmp.height}, .format = VK_FORMAT_R8_UNORM};
    shared_image dst = std::make_shared<image>(_display, img_params);

    // Use an updator to alloc a staging buffer big enough to update the whole image
    image::updator updator = dst->update();

    bitmapref_from_somelib bmpref {
      mybmp.width, mybmp.height,
      updator.staging_row_pitch(),
      updator.data()};

    // Let the lib directly rasterize/decode or whatever to the staging buffer
    somelib_do_something(bmpref);

    // Upon leaving the scope, the updator is destructed, the upload from staging to GPU is scheduled,
    // and the staging area will be freed after the upload is done.
