# Advanced Image Analysis Capabilities for Trakr Native

## Comprehensive Analysis Features Available

The `os_input_runner.py` tool provides sophisticated multi-dimensional image analysis:

### 1. Multi-Metric Evaluation
- **Total Pixel Similarity %** - Overall RGB pixel match (threshold ≤24.0 error)
- **FG Match %** - Foreground object alignment (pixels with max(R,G,B) > 20)
- **SSIM %** - Structural Similarity Index (8×8 block analysis)
- **MAE %** - Mean Absolute Error (average RGB difference)

### 2. Semantic Band Analysis (`[BANDS]`)
Breaks down errors by vertical regions:
- **HeaderBar (y=0..30)** - Top status bar area
- **UpperBody (y=30..230)** - Main content upper half
- **LowerBody (y=230..430)** - Main content lower half  
- **SoftButtons (y=430..480)** - Bottom 8 softkey buttons

### 3. Hotspot Detection (`[HOTSPOT #1..5]`)
Identifies top 5 mismatch bounding boxes with:
- Exact rectangle coordinates (x0,y0..x1,y1)
- Pixel loss percentage and count
- Dominant mismatch colors in IM_COL32(R,G,B,255) format
- **Shift Hints** - Optimal (dx, dy) translation to recover pixels

### 4. Coordinate-Based Telemetry
```
[HOTSPOT #1] rect=(196,56..276,80) loss=-0.42% (2016px) 
| Emu=IM_COL32(48,203,63,255) vs App=IM_COL32(42,180,55,255) 
| SHIFT HINT: move App (dx=+1, dy=0) -> recovers +0.18%
```

## Recommended Analysis Workflow

### Step 1: Fast Hotspot Analysis
```bash
# 2-second analysis with detailed telemetry
python3 native/tools/os_input/os_input_runner.py --only perform --no-emu --hotspots
```

### Step 2: Interpret Hotspot Telemetry
1. **Check band distribution** - Where are errors concentrated?
2. **Exact coordinates** - Use FT812 reference functions for precise alignment
3. **Color analysis** - Compare IM_COL32 values with reference implementation
4. **Shift hints** - Apply suggested (dx, dy) translations

### Step 3: Coordinate-Based Refinement
Never guess coordinates - use exact references:
- **Perform**: `renderPerformRef` in `FT812ScreensCore.cpp:175-232`
- **Song**: `renderSongRef` in `FT812ScreensCore.cpp:126-173`  
- **Config**: `renderConfigRef` in `FT812ScreensExtra.cpp:146-198`
- **Sample**: `renderSampleRef` in `FT812ScreensExtra.cpp:54-97`
- **Instrument**: `renderInstrumentRef` in `FT812ScreensExtra.cpp:10-52`

### Step 4: Validation Loop
```bash
# After each adjustment, verify improvement
python3 native/tools/os_input/os_input_runner.py --only <screen> --no-emu --hotspots

# Full validation after significant changes
python3 native/tools/os_input/os_input_runner.py
./native/build/trakr_app --auto-test
./native/build/trakr_tests
```

## Priority Analysis Targets

### Perform Screen (93.09% → needs +1.91%)
- Focus on: Track headers (T1..T8 at 4 + t * 99, y=30..64)
- Effect rows (y = 70 + r * 66)
- Active green indicator bars
- Bottom underline positioning (y ≤ 429)

### Song Screen (93.77% → needs +1.23%)
- Focus on: Table header row (y=30..52)
- Track column x-offsets (196 + t * 75)
- Slot row spacing (56 + row * 24)
- Active cursor box thickness/padding

### Sample Screen (94.18% → needs +0.82%)
- **CRITICAL**: Maintain solid #30CB3F green waveform fill
- Bottom parameter boxes (Start, End, Loop Start, Loop End)
- Marker flags (S, LS, LE, E) coordinates

The analysis tools provide surgical precision for visual refinement - use the hotspot telemetry and coordinate references for exact pixel-perfect alignment.