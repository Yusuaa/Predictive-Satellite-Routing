#!/bin/bash

# Build the image
echo "🐳 Building Docker image (this may take a while)..."
docker build -t ns3-dce-env .

# Run the container
# We mount the current directory (satellite-leo-rfp) to scratch/satnet-rfp inside the container
# The patch script is also mounted and executed
echo "🚀 Running container..."
docker run -it --rm \
    -v $(pwd):/workspace/source/ns-3-dce/myscripts/satnet-rfp \
    ns3-dce-env \
    /bin/bash /workspace/source/ns-3-dce/myscripts/satnet-rfp/scripts/docker_patch.sh
