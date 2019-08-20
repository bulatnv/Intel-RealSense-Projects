This sample streams depth images with applyed filters (Sub-sampling (Decimation), Threshhold, Spatial, Temporal, Hole Filling). All filters applyed in the main thread.


### Building
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j4
```

### Running 
```sh
./Depth-Post-Processing
```
Press any botton to stop.
