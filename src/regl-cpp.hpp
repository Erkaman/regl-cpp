#pragma once

#include <string>
#include <array>
#include <vector>
#include <functional>
#include <stack>
#include <map>

namespace reglCpp
{

struct uniformValue {
	union {
		std::array<float, 1> mGlFloatVec1;
		std::array<float, 2> mGlFloatVec2;
		std::array<float, 3> mGlFloatVec3;
		std::array<float, 4> mGlFloatVec4;

		std::array<std::array<float, 4>, 4 > mGlFloatMat4x4;
	};

	uniformValue(float v0) {
		this->mGlFloatVec1[0] = v0;

	}

	uniformValue(float v0, float v1) {
		this->mGlFloatVec2[0] = v0;
		this->mGlFloatVec2[1] = v1;
	}

	uniformValue(float v0, float v1, float v2) {
		this->mGlFloatVec3[0] = v0;
		this->mGlFloatVec3[1] = v1;
		this->mGlFloatVec3[2] = v2;
	}

	uniformValue(float v0, float v1, float v2, float v3) {
		this->mGlFloatVec4[0] = v0;
		this->mGlFloatVec4[1] = v1;
		this->mGlFloatVec4[2] = v2;
		this->mGlFloatVec4[3] = v3;
	}

	uniformValue(const std::array<std::array<float, 4>, 4 > & glFloatMat4x4) {
		this->mGlFloatMat4x4 = glFloatMat4x4;
	}

	uniformValue() {}
};

struct uniform {
	std::string mKey;
	uniformValue mValue;
};

struct pass {
	std::array<float, 4> mClearColor = {NAN, NAN, NAN, NAN};
	float mClearDepth = NAN;

	pass& clearColor(const std::array<float, 4>& clearColor) {
		this->mClearColor = clearColor;
		return *this;
	}

	pass& clearDepth(float clearDepth) {
		this->mClearDepth = clearDepth;
		return *this;
	}
};

struct pipeline {
	// .first contains the actual value. 
	// .second specifies whether this value has been actually set. If second==false, treat 'first' as invalid.
	// since it hasnt been set yet. 
	std::pair<bool, bool> mDepthTest = {false, false};
	
	std::string mVert = "";
	std::string mFrag = "";
	
	pipeline& depthTest(bool depthTest) {
		this->mDepthTest.first = depthTest;
		this->mDepthTest.second = true;
		return *this;
	}

	pipeline& vert(const std::string& vert) {
		this->mVert = vert;
		return *this;
	}

	pipeline& frag(const std::string& frag) {
		this->mFrag = frag;
		return *this;
	}
};

struct vertexBuffer {
	// should probably be a pointer to data instead.
	std::vector<float> mData;
	int mLength = -1; // if -1, just use length of mData.
	
	vertexBuffer& data(const std::vector<float> data) {
		mData = data;
		return *this;
	}
	
	vertexBuffer& length(int length) {
		mLength = length;
		return *this;
	}
	
	vertexBuffer& finish() {
		// TODO: actually allocate GL data.
		return *this;
	};
};

struct indexBuffer {
	// should probably be a pointer to data instead.
	std::vector<int> mData;
	int mLength = -1; // if -1, just use length of mData.

	indexBuffer& data(const std::vector<int> data) {
		mData = data;
		return *this;
	}

	indexBuffer& length(int length) {
		mLength = length;
		return *this;
	}

	indexBuffer& finish() {
		// TODO: actually allocate GL data.
		return *this;
	};
};

struct attribute {
	std::string mKey;
	vertexBuffer* mVertexBuffer;
};

struct command {
	pass mPass;
	pipeline mPipeline;
	std::vector<uniform> mUniforms;
	std::vector<attribute> mAttributes;
	indexBuffer* mIndices = nullptr;
	int mCount = -1;

	command& pass(const pass& pass) {
		this->mPass = pass;
		return *this;
	}

	command& uniforms(const std::vector<uniform>& uniforms) {
		this->mUniforms = uniforms;
		return *this;
	}

	command& count(const int count) {
		this->mCount = count;
		return *this;
	}

	command& attributes(const std::vector<attribute>& attributes) {
		this->mAttributes = attributes;
		return *this;
	}

	command& indices(indexBuffer* indices) {
		this->mIndices = indices;
		return *this;
	}
	command& pipeline(const pipeline& pipeline) {
		this->mPipeline = pipeline;
		return *this;
	}
};

struct reglCppContext {
private:

	struct contextState{
		// pass
		std::array<float, 4> mClearColor;
		float mClearDepth;

		// pipeline.
		std::string mVert;
		std::string mFrag;
		bool mDepthTest;
		
		// command
		std::map<std::string, uniformValue> mUniforms;		
		std::map<std::string, vertexBuffer*> mAttributes;
		indexBuffer* mIndices;
		int mCount;
		
		contextState(){
			mClearColor = { NAN, NAN, NAN, NAN };
			mClearDepth = NAN;


			mVert = "";
			mFrag = "";

			mDepthTest = true;

			mCount = 0;
		}
	};

	struct ProgramInfo {
		unsigned int mProgram;

		std::map<std::string, unsigned int> mUniforms;
		std::map<std::string, int> mAttributes;
	};
	std::map<std::string, ProgramInfo> programCache;

	ProgramInfo fetchProgram(const std::string& vert, const std::string& frag);

	void transferStack(contextState& stackState, const command& cmd);


	std::stack<contextState> stateStack;

public:
	void frame(const std::function<void()>& fn);

	void submit(const pass& cmd);
	void submit(const command& cmd);


};

extern reglCppContext context;


}