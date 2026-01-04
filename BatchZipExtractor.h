#pragma once
#ifndef BATCH_ZIP_EXTRACTOR_H
#define BATCH_ZIP_EXTRACTOR_H

#include <string>

class BatchZipExtractor {
public:
    static bool splitExtract(const std::string& zipPath, const std::string& outputDir1, const std::string& outputDir2);
};

#endif // BATCH_ZIP_EXTRACTOR_H