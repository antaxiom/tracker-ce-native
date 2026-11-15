# Visual Review Workflow for Trakr Native

## Recommended Approach for Visual Refinement

### 1. Fast Iteration Loop
```bash
# Single-screen testing with hotspot analysis (< 2 seconds)
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots
```

### 2. Multi-Metric Analysis
The tool provides:
- **Total Pixel Similarity %** - Overall match
- **FG Match %** - Foreground object alignment  
- **SSIM %** - Structural similarity
- **MAE %** - Mean absolute error
- **Hotspot bounding boxes** - Top 5 mismatch areas with exact coordinates

### 3. Coordinate-Based Alignment Strategy

**Never guess coordinates** - Use the exact FT812 reference functions:

| Screen | Reference Function | Native View File |
|--------|-------------------|------------------|
| Song | `renderSongRef` (FT812ScreensCore.cpp:126-173) | OrderListView.cpp |
| Config | `renderConfigRef` (FT812ScreensExtra.cpp:146-198) | ConfigView.cpp |
| Sample | `renderSampleRef` (FT812ScreensExtra.cpp:54-97) | SampleView.cpp |
| Instrument | `renderInstrumentRef` (FT812ScreensExtra.cpp:10-52) | InstrumentView.cpp |
| Perform | `renderPerformRef` (FT812ScreensCore.cpp:175-232) | PerformView.cpp |

### 4. Common Alignment Targets
- **HeaderBar**: Text vertically centered in y=0..26 band
- **ScreenButtons**: Bottom softkeys at y=434..480, 99px wide with 1px gaps
- **Waveforms**: Solid #30CB3F green fill with no interior patterns
- **Grid coordinates**: Use exact x/y/w/h from reference functions

### 5. Validation Process
1. Make coordinate adjustment
2. Run single-screen test
3. Check hotspot telemetry for exact mismatch details
4. Verify no regression in other metrics
5. Repeat until ≥95.0% achieved

### 6. Batch Testing
After significant changes, run full validation:
```bash
# Generate all reference images
python3 native/tools/os_input/os_input_runner.py

# Run full robot audit
./native/build/trakr_app --auto-test

# Verify unit tests
./native/build/trakr_tests
```