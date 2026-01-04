#pragma once
#ifndef BATCH_ZIP_COMPRESSOR_H
#define BATCH_ZIP_COMPRESSOR_H

#include <string>
#include <vector>

class BatchZipCompressor {
public:
    static bool batchCompress(const std::vector<std::string>& sourceDirs, const std::string& outputZipPath);
};

#endif // BATCH_ZIP_COMPRESSOR_H