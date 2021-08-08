if (HUT_ENABLE_IMGUI)
  add_subdirectory(imgui)
endif()

if (HUT_ENABLE_KTX2)
  add_subdirectory(ktx2)
endif()

if (HUT_ENABLE_RENDERDOC)
  add_subdirectory(renderdoc)
endif()

if (HUT_ENABLE_IMGDEC)
  add_subdirectory(imgdec)
endif()
