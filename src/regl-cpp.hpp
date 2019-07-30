#pragma once

#include <string>
#include <array>
#include <vector>
#include <functional>
#include <stack>
#include <map>


namespace reglCpp
{

struct Texture2D;
	
struct UniformValue {
	
	enum UniformType {
		FLOAT_VEC1,
		FLOAT_VEC2,
		FLOAT_VEC3,
		FLOAT_VEC4,

		FLOAT_MAT4X4,

		TEXTURE2D,

		UNSET
	};
	
	union {
		std::array<float, 1> mFloatVec1;
		std::array<float, 2> mFloatVec2;
		std::array<float, 3> mFloatVec3;
		std::array<float, 4> mFloatVec4;

		std::array<std::array<float, 4>, 4 > mFloatMat4x4;

		Texture2D* mTexture2D;
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
	
	UniformValue(Texture2D* texture2D) {
		this->mTexture2D = texture2D;
		mType = TEXTURE2D;
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
	float* mData = nullptr;
	
	// gl buffer object.
	std::pair<unsigned int, bool> mBufferObject = { -1, false };
	
	int mNumComponents = -1; // the numer of components of each element in the buffer. 3, means VEC3 for instance
	int mLength = -1; // the number of elements in the buffer

	/*
	either 'static', 'dynamic' or 'stream'
	*/
	std::string mUsage = "static";

	std::string mName = "unnnamed"; // can be useful setting for debugging.
	
	VertexBuffer& data(float* data) {
		mData = data;
		return *this;
	}
	
	VertexBuffer& length(int length) {
		mLength = length;
		return *this;
	}
	
	VertexBuffer& numComponents(int numComponents) {
		mNumComponents = numComponents;
		return *this;
	}

	VertexBuffer& usage(const std::string& usage) {
		mUsage = usage;
		return *this;
	}
		
	VertexBuffer& name(const std::string& name) {
		mName = name;
		return *this;
	}

	VertexBuffer& finish();
	void dispose();
};

struct IndexBuffer {
	// should probably be a pointer to data instead.
	unsigned int* mData = nullptr;

	std::pair<unsigned int, bool> mBufferObject = { -1, false };
	int mLength = -1; 
	std::string mName = "unnnamed"; // can be useful setting for debugging.

	/*
	either 'static', 'dynamic' or 'stream'
	*/
	std::string mUsage = "static";

	IndexBuffer& data(unsigned int* data) {
		mData = data;
		return *this;
	}

	IndexBuffer& length(int length) {
		mLength = length;
		return *this;
	}

	IndexBuffer& usage(const std::string& usage) {
		mUsage = usage;
		return *this;
	}

	IndexBuffer& name(const std::string& name) {
		mName = name;
		return *this;
	}

	IndexBuffer& finish();	
	void dispose();
};

struct Texture2D {
	// gl buffer object.
	std::pair<unsigned int, bool> mTexture = { -1, false };

	// should probably be a pointer to data instead.
	float* mFloatData = nullptr;
	unsigned char* mCharData = nullptr;

	int mWidth = -1;
	int mHeight = -1;
	
	std::string mMag = "nearest";
	std::string mMin = "nearest";
	
	std::string mWrapS = "clamp";
	std::string mWrapT = "clamp";

	std::string mPixelFormat = "rgba8";

	std::string mName = "unnnamed"; // can be useful setting for debugging.
	
	Texture2D& data(unsigned char* data) {
		mCharData = data;
		mFloatData = nullptr;
		return *this;
	}

	Texture2D& data(float* data) {
		mCharData = nullptr;
		mFloatData = data;
		return *this;
	}
	Texture2D& width(int width) {
		mWidth = width;
		return *this;
	}

	Texture2D& height(int height) {
		mHeight = height;
		return *this;
	}

	Texture2D& mag(const std::string& mag) {
		mMag = mag;
		return *this;
	}

	Texture2D& min(const std::string& min) {
		mMin = min;
		return *this;
	}

	Texture2D& wrapS(const std::string& wrapS) {
		mWrapS = wrapS;
		return *this;
	}

	Texture2D& wrapT(const std::string& wrapT) {
		mWrapT = wrapT;
		return *this;
	}

	Texture2D& wrap(const std::string& wrap) {
		mWrapS = wrap;
		mWrapT = wrap;
		return *this;
	}

	Texture2D& pixelFormat(const std::string& pixelFormat) {
		mPixelFormat = pixelFormat;
		return *this;
	}

	Texture2D& name(const std::string& name) {
		mName = name;
		return *this;
	}

	Texture2D& finish();
	void dispose();
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
	std::array<int, 4> mViewport = {-1, -1, -1, -1};

	std::array<float, 4> mClearColor = { NAN, NAN, NAN, NAN };
	float mClearDepth = NAN;
	// .first contains the actual value. 
	// .second specifies whether this value has been actually set. If second==false, treat 'first' as invalid.
	// since it hasnt been set yet. 
	std::pair<bool, bool> mDepthTest = { false, false };
	std::string mVert = "";
	std::string mFrag = "";
	
	Command& viewport(int x, int y, int w, int h) {
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
		std::array<int, 4> mViewport;
		
		contextState(){
			mViewport = { -1, -1, -1, -1 };

			mClearColor = { NAN, NAN, NAN, NAN };
			mClearDepth = NAN;

			mVert = "";
			mFrag = "";

			mDepthTest = true;

			mCount = -1;
		}
	};

	struct ProgramInfo {
		unsigned int mProgram = -1;

		std::string mVert = "";
		std::string mFrag = "";

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
	void submit(const Command& command, const std::function<void()>& fn);
	
	void dispose();
};

extern reglCppContext context;


}