#include "trt_builder.h"
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include "buffers.h"
#include "calibrator.h"

void buildEngine(buildConf cfg)
{
    const char *onnx_file_path = cfg.onnx_model_path.c_str();

	auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(sample::gLogger.getTRTLogger()));
    if (!builder)
    {
        sample::gLogError << "Failed to create builder" << std::endl;
        return;
    }
    // ����batch
    const auto explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(builder->createNetworkV2(explicitBatch));
    if (!network)
    {
        sample::gLogError << "Failed to create network" << std::endl;
        return ;
    }

    // ����onnxparser�����ڽ���onnx�ļ�
    auto parser = std::unique_ptr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, sample::gLogger.getTRTLogger()));
    // ����onnxparser��parseFromFile��������onnx�ļ�
    auto parsed = parser->parseFromFile(onnx_file_path, static_cast<int>(sample::gLogger.getReportableSeverity()));
    if (!parsed)
    {
        sample::gLogError << "Failed to parse onnx file" << std::endl;
        return ;
    }
    // ��ȡ����ڵ�ά����Ϣ
    nvinfer1::ITensor* inputTensor = network->getInput(0);
    nvinfer1::Dims inputDims = inputTensor->getDimensions();

    // ========== 3. ����config���ã�builder--->config ==========
    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    if (!config)
    {
        sample::gLogError << "Failed to create config" << std::endl;
        return;
    }
    
    if (inputDims.d[0] == -1)
    {
        auto profile = builder->createOptimizationProfile();                                                           // ����profile��������������Ķ�̬�ߴ�
        profile->setDimensions(inputTensor->getName(), nvinfer1::OptProfileSelector::kMIN, nvinfer1::Dims4{ 1, 3, cfg.input_height, cfg.input_width }); // ������С�ߴ�
        profile->setDimensions(inputTensor->getName(), nvinfer1::OptProfileSelector::kOPT, nvinfer1::Dims4{ cfg.batch_size, 3, cfg.input_height, cfg.input_width }); // �������ųߴ�
        profile->setDimensions(inputTensor->getName(), nvinfer1::OptProfileSelector::kMAX, nvinfer1::Dims4{ cfg.batch_size, 3, cfg.input_height, cfg.input_width }); // �������ߴ�
        // ʹ��addOptimizationProfile�������profile��������������Ķ�̬�ߴ�
        config->addOptimizationProfile(profile);
    }
    
    if (cfg.mode == Mode::FP16)
    {
        if (builder->platformHasFastFp16())
        {
            config->setFlag(nvinfer1::BuilderFlag::kFP16);
        }
        else {
            sample::gLogError << "device don't support fp16" << std::endl;
            return;
        }
    }
    
    if (cfg.mode == Mode::INT8)
    {
        if (cfg.dataDir.empty())
        {
            sample::gLogError << "dataDir is empty!" << std::endl;
            return;
        }
        if (builder->platformHasFastInt8()) {
            auto calibrator = new Int8EntropyCalibrator2(cfg);
            config->setFlag(nvinfer1::BuilderFlag::kINT8);
            config->setInt8Calibrator(calibrator);
        }
        else
        {
            sample::gLogError << "device don't support int8." << std::endl;
            return;
        }
    }

    // �������batchsize
    builder->setMaxBatchSize(cfg.batch_size);
    // ����������ռ䣨�°汾��TensorRT�Ѿ�������setWorkspaceSize��
    config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, cfg.maxWorkspaceSize);

    // ����������������profile
    auto profileStream = samplesCommon::makeCudaStream();
    if (!profileStream)
    {
        sample::gLogError << "Failed to create stream" << std::endl;
        return;
    }
    config->setProfileStream(*profileStream);

    auto plan = std::unique_ptr<nvinfer1::IHostMemory>(builder->buildSerializedNetwork(*network, *config));
    if (!plan)
    {
        sample::gLogError << "Failed to create engine" << std::endl;
        return;
    }

    std::ofstream engine_file(cfg.trt_model_save_path, std::ios::binary);
    assert(engine_file.is_open() && "Failed to open engine file");
    engine_file.write((char*)plan->data(), plan->size());
    engine_file.close();

    sample::gLogInfo << "Engine build success!" << std::endl;
    return;
}
