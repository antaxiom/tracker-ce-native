# Trakr Native Commands

## Build & Test
- `cmake --build native/build -j8` - Build application and tests
- `./native/build/trakr_tests` - Run unit tests (103 test cases)
- `./native/build/trakr_app --auto-test` - Run UI robot audit (1,103 frames)

## Visual Testing
- `python3 native/tools/os_input/os_input_runner.py` - Generate FT812 reference images
- `python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots` - Fast single-screen testing

## File Management
- `find native/app native/audio native/model native/engine native/io native/tests -name "*.cpp" -o -name "*.h" | xargs wc -l | sort -nr | head -n 15` - Check file line counts