# Intel-RealSense-Projects

### 1 - Color-Streamig
Sample code streaming color frames from sensor

### 2 - Depth-Streamig
Sample code streaming depth frames from sensor

### 3 - Depth-Post-processing
Sample code for streaming depth frames with applyed filter (Sub-sampling (Decimation), Threshhold, Spatial, Temporal, Hole Filling). All filters applyed in the main thread.

### 4 - Threads-Depth-Post-processing
Almoust the same example as Depth-Post-processing. In this sample all filters applyed in separate thread.

### 5 - Depth-to-png
Sample code for saving depth frames to .png files

### 6 - Depth-and-Color-to-png.
Sample code for saving depth and color frames to .png files at the same time

## Building and Running
Building and running Depth-Streamig for example:

### Building
```sh
cd Depth-Streaming
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j4
```

### Running 
```sh
./DepthStreaming 
```
Press any botton to stop.
