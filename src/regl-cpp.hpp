#pragma once

#include <string>
#include <array>
#include <vector>
#include <functional>
#include <stack>
#include <map>

namespace reglCpp
{
	
struct UniformValue {
	
	enum UniformType {
		FLOAT_VEC1,
		FLOAT_VEC2,
		FLOAT_VEC3,
		FLOAT_VEC4,

		FLOAT_MAT4X4,

		UNSET
	};
	
	union {
		std::array<float, 1> mFloatVec1;
		std::array<float, 2> mFloatVec2;
		std::array<float, 3> mFloatVec3;
		std::array<float, 4> mFloatVec4;

		std::array<std::array<float, 4>, 4 > mFloatMat4x4;
	};
	UniformType mType = UNSET;

	UniformValue(float v0) {
		this->mFloatVec1[0] = v0;
		mType = FLOAT_VEC1;
	}

	UniformValue(float v0, float v1) {
		this->mFloatVec2[0] = v0;
		this->mFloatVec2[1] = v1;
		mType = FLOAT_VEC2;
	}

	UniformValue(float v0, float v1, float v2) {
		this->mFloatVec3[0] = v0;
		this->mFloatVec3[1] = v1;
		this->mFloatVec3[2] = v2;
		mType = FLOAT_VEC3;
	}

	UniformValue(float v0, float v1, float v2, float v3) {
		this->mFloatVec4[0] = v0;
		this->mFloatVec4[1] = v1;
		this->mFloatVec4[2] = v2;
		this->mFloatVec4[3] = v3;
		mType = FLOAT_VEC4;
	}

	UniformValue(const std::array<std::array<float, 4>, 4 > & floatMat4x4) {
		this->mFloatMat4x4 = floatMat4x4;
		mType = FLOAT_MAT4X4;
	}
	
	UniformValue() { 
		mType = UNSET; 
	}
};

struct Uniform {
	std::string mKey;
	UniformValue mValue;
};

struct VertexBuffer {
	// should probably be a pointer to data instead.
	std::vector<float> mData;
	int mLength = -1; // if -1, just use length of mData.
	
	VertexBuffer& data(const std::vector<float> data) {
		mData = data;
		return *this;
	}

	VertexBuffer& length(int length) {
		mLength = length;
		return *this;
	}
	
	VertexBuffer& finish() {
		// TODO: actually allocate GL data.
		return *this;
	};
};

struct IndexBuffer {
	// should probably be a pointer to data instead.
	std::vector<int> mData;
	int mLength = -1; // if -1, just use length of mData.

	IndexBuffer& data(const std::vector<int> data) {
		mData = data;
		return *this;
	}

	IndexBuffer& length(int length) {
		mLength = length;
		return *this;
	}

	IndexBuffer& finish() {
		// TODO: actually allocate GL data.
		return *this;
	};
};

struct Attribute {
	std::string mKey;
	VertexBuffer* mVertexBuffer;
};

struct Command {
	std::vector<Uniform> mUniforms;
	std::vector<Attribute> mAttributes;
	IndexBuffer* mIndices = nullptr;
	int mCount = -1;
	
	// x, y, w, h
	std::array<float, 4> mViewport = {NAN, NAN, NAN, NAN};

	std::array<float, 4> mClearColor = { NAN, NAN, NAN, NAN };
	float mClearDepth = NAN;
	// .first contains the actual value. 
	// .second specifies whether this value has been actually set. If second==false, treat 'first' as invalid.
	// since it hasnt been set yet. 
	std::pair<bool, bool> mDepthTest = { false, false };
	std::string mVert = "";
	std::string mFrag = "";
	
	Command& viewport(float x, float y, float w, float h) {
		mViewport[0] = x;
		mViewport[1] = y;
		mViewport[2] = w;
		mViewport[3] = h;
		return *this;
	}
	
	Command& uniforms(const std::vector<Uniform>& uniforms) {
		this->mUniforms = uniforms;
		return *this;
	}

	Command& count(const int count) {
		this->mCount = count;
		return *this;
	}

	Command& attributes(const std::vector<Attribute>& attributes) {
		this->mAttributes = attributes;
		return *this;
	}

	Command& indices(IndexBuffer* indices) {
		this->mIndices = indices;
		return *this;
	}
	
	Command& clearColor(const std::array<float, 4> & clearColor) {
		this->mClearColor = clearColor;
		return *this;
	}

	Command& clearDepth(float clearDepth) {
		this->mClearDepth = clearDepth;
		return *this;
	}

	Command& depthTest(bool depthTest) {
		this->mDepthTest.first = depthTest;
		this->mDepthTest.second = true;
		return *this;
	}

	Command& vert(const std::string& vert) {
		this->mVert = vert;
		return *this;
	}

	Command& frag(const std::string& frag) {
		this->mFrag = frag;
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
		std::map<std::string, UniformValue> mUniforms;		
		std::map<std::string, VertexBuffer*> mAttributes;
		IndexBuffer* mIndices;
		int mCount;
		std::array<float, 4> mViewport;
		
		contextState(){
			mViewport = { NAN, NAN, NAN, NAN };

			mClearColor = { NAN, NAN, NAN, NAN };
			mClearDepth = NAN;

			mVert = "";
			mFrag = "";

			mDepthTest = true;

			mCount = -1;
		}
	};

	struct ProgramInfo {
		unsigned int mProgram;

		std::string mVert;
		std::string mFrag;

		std::map<std::string, unsigned int> mUniforms;
		std::map<std::string, int> mAttributes;
	};
	std::map<std::string, ProgramInfo> programCache;

	ProgramInfo fetchProgram(const std::string& vert, const std::string& frag);

	void transferStack(contextState& stackState, const Command& command);


	std::stack<contextState> stateStack;

	void submitWithContextState(contextState state);

public:
	void frame(const std::function<void()>& fn);

	//void submit(const Pass& pass);
	void submit(const Command& command);
};

extern reglCppContext context;


}