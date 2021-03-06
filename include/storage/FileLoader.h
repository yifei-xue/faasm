#pragma once

#include <proto/faasm.pb.h>

#include <string>
#include <vector>
#include <util/exception.h>


namespace storage {
    class FileLoader {
    public:
        virtual std::vector<uint8_t> loadFunctionWasm(const message::Message &msg) = 0;

        virtual std::vector<uint8_t> loadSharedObjectWasm(const std::string &path) = 0;

        virtual std::vector<uint8_t> loadFunctionObjectFile(const message::Message &msg) = 0;

        virtual std::vector<uint8_t> loadSharedObjectObjectFile(const std::string &path) = 0;

        virtual std::vector<uint8_t> loadSharedFile(const std::string &path) = 0;

        virtual void uploadFunction(message::Message &msg) = 0;

        virtual void uploadPythonFunction(message::Message &msg) = 0;

        virtual void uploadFunctionObjectFile(const message::Message &msg, const std::vector<uint8_t> &objBytes) = 0;

        virtual void uploadSharedObjectObjectFile(const std::string &path, const std::vector<uint8_t> &objBytes) = 0;

        virtual void uploadSharedFile(const std::string &path, const std::vector<uint8_t> &fileBytes) = 0;

        bool isWasm(const std::vector<uint8_t> &bytes);

        void codegenForFunction(message::Message &msg);

        void codegenForSharedObject(const std::string &inputPath);
    protected:
        std::vector<uint8_t> doCodegen(std::vector<uint8_t> &bytes);
    };

    FileLoader &getFileLoader();

    void checkFileExists(const std::string &path);

    std::vector<uint8_t> loadFileBytes(const std::string &path);

    class SharedFileIsDirectoryException : public util::FaasmException {
    public:
        explicit SharedFileIsDirectoryException(const std::string& filePath): FaasmException(filePath + " is a directory") {

        }
    };
};