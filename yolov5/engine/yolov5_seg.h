#pragma once
#include <NvInfer.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "utils/config.h"


class YOLO_SEG
{
public:
	YOLO_SEG(Config cfg);
	~YOLO_SEG();

	virtual bool init();

public:
	virtual void preprocess(const cv::Mat& img);
	virtual void preprocess(const std::vector<cv::Mat>& imgsBatch);

	virtual bool infer();

	virtual void postprocess(std::vector<Detection>& res);
	virtual void postprocess(std::vector<std::vector<Detection>>& res);

protected:
	std::unique_ptr<nvinfer1::IRuntime> m_runtime;
	std::shared_ptr<nvinfer1::ICudaEngine> m_engine;
	std::unique_ptr<nvinfer1::IExecutionContext> m_context;

protected:
	Config m_cfg;
	bool m_usePlugin{ false };

	// ģ�������Ϣ
	nvinfer1::Dims m_detect_dims;
	nvinfer1::Dims m_proto_dims;

	cudaStream_t m_stream;

	std::vector<AffineMatrix> m_dst2src;

	int m_classes_nums; // �����

	int m_total_objects; // �������Ŀ������yolov5 80*80*3+40*40*3+20*20*3��
	int m_output_area; 
	int m_proto_area;

	int m_topK; // �������

	int m_output_objects_width;

protected:
	// ����������������ݴ洢��ַ
	void* m_buffers[3];

	float* m_output_objects_device;
	float* m_output_objects_host;
	float* m_output_seg_host;

};
