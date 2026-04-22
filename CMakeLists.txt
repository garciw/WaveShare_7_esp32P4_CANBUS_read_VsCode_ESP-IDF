# 1. Find all your custom logic .cpp files in 'main'
file(GLOB_RECURSE MAIN_SOURCES "*.cpp" "*.c")

# 2. Reach into the 'ui' folder and grab everything (screens, fonts, images)
file(GLOB_RECURSE UI_SOURCES 
    "ui/*.c"
    "ui/screens/*.c"
    "ui/fonts/*.c"
    "ui/images/*.c"
    "ui/components/*.c"
)

# 3. Register it all as one component (NO REQUIRES LIST!)
idf_component_register(
    SRCS 
        ${MAIN_SOURCES}
        ${UI_SOURCES}
    INCLUDE_DIRS 
        "." 
        "ui"
        "ui/screens" 
        "ui/fonts" 
        "ui/images"
        "ui/components"
)