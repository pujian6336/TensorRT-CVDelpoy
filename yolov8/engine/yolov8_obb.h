#pragma once
#include <NvInfer.h>
#include <opencv2/opencv.hpp>

#include "utils/config.h"


class YOLO_OBB
{
public:
	YOLO_OBB(Config cfg);
	~YOLO_OBB();

	virtual bool init();

	virtual void Run(const cv::Mat& img, std::vector<OBBResult>& res);
	virtual void Run(const std::vector<cv::Mat>& imgsBatch, std::vector<std::vector<OBBResult>>& res);

	virtual void warmUp(int epoch = 10);

public:
	virtual void preprocess(const cv::Mat& img);
	virtual void preprocess(const std::vector<cv::Mat>& imgsBatch);

	virtual bool infer();

	virtual void postprocess(std::vector<OBBResult>& res);
	virtual void postprocess(std::vector<std::vector<OBBResult>>& res);
protected:
	std::unique_ptr<nvinfer1::IRuntime> m_runtime;
	std::shared_ptr<nvinfer1::ICudaEngine> m_engine;
	std::unique_ptr<nvinfer1::IExecutionContext> m_context;

protected:
	Config m_cfg;

	bool m_usePlugin{ false };

	int m_topK; // �������
	int m_imgSize;

	// ģ�������Ϣ
	nvinfer1::Dims m_output_dims;
	int m_classes_nums; // �����
	int m_total_objects; // �������Ŀ������yolov5 80*80*3+40*40*3+20*20*3��
	int m_output_area; //

	std::vector<AffineMatrix> m_dst2src;

	cudaStream_t m_stream;

protected:

	// ����������������ݴ洢��ַ
	void* m_buffers[2];

	// ���
	float* m_output_objects_device;
	float* m_output_objects_host;
	int m_output_objects_width; // 7:left, top, right, bottom, scores, label, keepflag(nms���˺��Ƿ�����־);

};
