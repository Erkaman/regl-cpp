#include "regl-cpp.hpp"

#ifdef EMSCRIPTEN
#include<emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)

inline void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("OpenGL error %08x, at %s:%i - for %s.\n", err, fname, line, stmt);
	}
}

#ifdef NDEBUG
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
    } while (0)
#else
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
	CheckOpenGLError(#stmt, __FILE__, __LINE__);	\
    } while (0)
#endif


namespace reglCpp {

reglCppContext context;

void reglCppContext::frame(const std::function<void()>& fn) {
	contextState initialState;
	stateStack.push(initialState);
	fn();
}

void reglCppContext::transferStack(contextState& stackState, const Command& command) {
	if (command.mIndices != nullptr) {
		stackState.mIndices = command.mIndices;
	}
	if (command.mCount != -1) {
		stackState.mCount = command.mCount;
	}
	for (Attribute attribute : command.mAttributes) {
		stackState.mAttributes[attribute.mKey] = attribute.mVertexBuffer;
	}

	for (Uniform uniform : command.mUniforms) {
		stackState.mUniforms[uniform.mKey] = uniform.mValue;
	}

	if (command.mDepthTest.second) {
		stackState.mDepthTest = command.mDepthTest.first;
	}

	if (command.mVert != "") {
		stackState.mVert = command.mVert;
	}

	if (command.mFrag != "") {
		stackState.mFrag = command.mFrag;
	}

	if (command.mPrimitive != "") {
		stackState.mPrimitive = command.mPrimitive;
	}
	
	if (
		!isnan(command.mClearColor[0]) &&
		!isnan(command.mClearColor[1]) &&
		!isnan(command.mClearColor[2]) &&
		!isnan(command.mClearColor[3])
		) {
		stackState.mClearColor = command.mClearColor;
	}
	
	if (
		command.mViewport[0] != -1 &&
		command.mViewport[1] != -1 &&
		command.mViewport[2] != -1 &&
		command.mViewport[3] != -1) {
		stackState.mViewport = command.mViewport;
	}
	
	if (!isnan(command.mClearDepth)) {
		stackState.mClearDepth = command.mClearDepth;
	}

}

VertexBuffer& VertexBuffer::finish() {
	int glUsage;
	
	if (mUsage == "static") {
		glUsage = GL_STATIC_DRAW;
	}
	else if (mUsage == "dynamic") {
		glUsage = GL_DYNAMIC_DRAW;
	}
	else if (mUsage == "stream") {
		glUsage = GL_STREAM_DRAW;
	}
	else {
		printf("'%s' is not a valid valid of vertex buffer 'usage'\n", mUsage.c_str());
		exit(1);
	}

	// 1, 2, 3, 4.
	if (mNumComponents <= 0 || mNumComponents >= 5) {
		printf("'%d' is not a valid  number of vertex buffer components\n", mNumComponents);
		exit(1);
	}
	
	if (mLength < 0) {
		printf("'%d' is not a valid vertex buffer length\n", mLength);
		exit(1);
	}

	if (mData == nullptr) {
		printf("Need to specify data for vertex buffer\n");
		exit(1);
	}

	GL_C(glGenBuffers(1, &mBufferObject.first));
	GL_C(glBindBuffer(GL_ARRAY_BUFFER, mBufferObject.first));
	GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mNumComponents * mLength, (float*)mData, glUsage));
	GL_C(glBindBuffer(GL_ARRAY_BUFFER, 0));
	mBufferObject.second = true; // signify it was properly finished.

	return *this;
};

void VertexBuffer::dispose() {
	GL_C(glDeleteBuffers(1, &mBufferObject.first));
	mBufferObject.second = false;
}

IndexBuffer& IndexBuffer::finish() {
	int glUsage;

	if (mUsage == "static") {
		glUsage = GL_STATIC_DRAW;
	}
	else if (mUsage == "dynamic") {
		glUsage = GL_DYNAMIC_DRAW;
	}
	else if (mUsage == "stream") {
		glUsage = GL_STREAM_DRAW;
	}
	else {
		printf("'%s' is not a valid valid of index buffer 'usage'\n", mUsage.c_str());
		exit(1);
	}

	if (mLength < 0) {
		printf("'%d' is not a valid index buffer length\n", mLength);
		exit(1);
	}
	
	if (mData == nullptr) {
		printf("Need to specify data for index buffer\n");
		exit(1);
	}
	
	GL_C(glGenBuffers(1, &mBufferObject.first));
	GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferObject.first));
	GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * mLength, (float*)mData, glUsage));
	GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	mBufferObject.second = true; // signify it was properly finished.
	
	return *this;
};

void IndexBuffer::dispose() {
	GL_C(glDeleteBuffers(1, &mBufferObject.first));
	mBufferObject.second = false;
}

Texture2D& Texture2D::finish() {

	if (mWidth < 0) {
		printf("'%d' is not a valid texture width\n", mWidth);
		exit(1);
	}

	if (mHeight < 0) {
		printf("'%d' is not a valid texture height\n", mHeight);
		exit(1);
	}

	
	auto strToWrap = [](const std::string& str) {
		if (str == "clamp") {
			return GL_CLAMP_TO_EDGE;
		} else if (str == "repeat") {
			return GL_REPEAT;
		} else if (str == "mirror") {
			return GL_MIRRORED_REPEAT;
		} else {
			return -1;
		}
	};

	int wrapS = strToWrap(mWrapS);
	if (wrapS == -1) {
		printf("'%s' is not a valid texture wrap mode\n", mWrapS.c_str());
		exit(1);
	}

	int wrapT = strToWrap(mWrapT);
	if (wrapS == -1) {
		printf("'%s' is not a valid texture wrap mode\n", mWrapT.c_str());
		exit(1);
	}

	int mag = -1;
	if (mMag == "nearest") {
		mag = GL_NEAREST;
	} else if (mMag == "linear") {
		mag = GL_LINEAR;
	} else {
		printf("'%s' is not a valid texture mag filter\n", mMag.c_str());
		exit(1);
	}

	int min = -1;
	bool genMipmap = false;
	if (mMin == "nearest") {
		min = GL_NEAREST;
	} else if (mMin == "linear") {
		min = GL_LINEAR;
	} else if (mMin == "linear mipmap linear") {
		min = GL_LINEAR_MIPMAP_LINEAR;
		genMipmap = true;
	} else if (mMin == "nearest mipmap linear") {
		min = GL_NEAREST_MIPMAP_LINEAR;
		genMipmap = true;
	} else if (mMin == "linear mipmap nearest") {
		min = GL_LINEAR_MIPMAP_NEAREST;
		genMipmap = true;
	} else {
		printf("'%s' is not a valid texture mag filter\n", mMag.c_str());
		exit(1);
	}

	GL_C(glGenTextures(1, &mTexture.first));
	GL_C(glBindTexture(GL_TEXTURE_2D, mTexture.first));
	
	if (mPixelFormat == "rgba8") {
		if (mCharData == nullptr) {
			printf("Need to specify 'unsigned char' array for pixel format %s\n", mPixelFormat.c_str());
			exit(1);
		}

		GL_C(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mCharData));
	}
	else {
		printf("Unsupported pixel format %s\n", mPixelFormat.c_str());
		exit(1);
	}
	
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT));

	if (genMipmap) {
		GL_C(glGenerateMipmap(GL_TEXTURE_2D));
	}

	GL_C(glBindTexture(GL_TEXTURE_2D, 0));

	mTexture.second = true;

	return *this;
}

void Texture2D::dispose() {
	GL_C(glDeleteTextures(1, &mTexture.first));
	mTexture.second = false;
}

inline char* GetShaderLogInfo(GLuint shader) {
	GLint len;
	GLsizei actualLen;
	GL_C(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
	char* infoLog = new char[len];
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));
	return infoLog;
}

inline GLuint CreateShaderFromString(const std::string& shaderSource, const GLenum shaderType) {
	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));
	const char* c_str = shaderSource.c_str();
	GL_C(glShaderSource(shader, 1, &c_str, NULL));
	GL_C(glCompileShader(shader));

	GLint compileStatus;
	GL_C(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
	if (compileStatus != GL_TRUE) {
		LOGI("Could not compile shader\n\n%s \n\n%s\n", shaderSource.c_str(),
			GetShaderLogInfo(shader));
		exit(1);
	}

	return shader;
}

inline GLuint LoadNormalShader(const std::string& vsSource, const std::string& fsShader) {

	bool useGl3 = true;// just hardcode this for now.

	std::string prefix = "";

	prefix += "#version 100\n";
	
	prefix += std::string(R"(

)");

	GLuint vs = CreateShaderFromString(prefix + vsSource, GL_VERTEX_SHADER);
	GLuint fs = CreateShaderFromString(prefix + fsShader, GL_FRAGMENT_SHADER);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint Result;
	glGetProgramiv(shader, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		LOGI("Could not link shader \n\n%s\n", GetShaderLogInfo(shader));
		exit(1);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
}

reglCppContext::ProgramInfo reglCppContext::fetchProgram(const std::string& vert, const std::string& frag){
	std::string key = vert + frag;
	
	if (programCache.count(key) != 0) {
		return programCache[key];
	}

	ProgramInfo programInfo;

	programInfo.mProgram = LoadNormalShader(vert, frag);
	programInfo.mVert = vert;
	programInfo.mFrag = frag;

	// get all attribs:
	{
		int count = 0;
		glGetProgramiv(programInfo.mProgram, GL_ACTIVE_ATTRIBUTES, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];
		
		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveAttrib(programInfo.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int attribLocation = 0;
			GL_C(attribLocation = glGetAttribLocation(programInfo.mProgram, nameStr.c_str()));

			programInfo.mAttributes[nameStr] = attribLocation;
		}
	}

	// get all uniforms:
	{
		int count = 0;
		glGetProgramiv(programInfo.mProgram, GL_ACTIVE_UNIFORMS, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];

		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveUniform(programInfo.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int uniformLocation = 0;
			GL_C(uniformLocation = glGetUniformLocation(programInfo.mProgram, nameStr.c_str()));

			programInfo.mUniforms[nameStr] = uniformLocation;
		}
	}
	return programInfo;
}



void reglCppContext::submitWithContextState(contextState state) {

	if (state.mViewport[0] == -1 ||
		state.mViewport[1] == -1 ||
		state.mViewport[2] == -1 ||
		state.mViewport[3] == -1) {
		
		printf("you need to specify a viewport for your command");
	}
	
	GL_C(glViewport(state.mViewport[0], state.mViewport[1], state.mViewport[2], state.mViewport[3]));

	bool doClear = !isnan(state.mClearColor[0]) &&
		!isnan(state.mClearColor[1]) &&
		!isnan(state.mClearColor[2]) &&
		!isnan(state.mClearColor[3]);

	doClear = doClear && !isnan(state.mClearDepth);
	
	// translate stack state to GL commands. 
	if (doClear) {

#ifdef EMSCRIPTEN
		GL_C(glClearDepthf(state.mClearDepth));
#else
		GL_C(glClearDepth(state.mClearDepth));
#endif

		GL_C(glClearColor(
			state.mClearColor[0],
			state.mClearColor[1],
			state.mClearColor[2],
			state.mClearColor[3]
		));
		
		GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	}

	//set view port  glViewport.

	if (state.mCount != -1) {
		if (state.mDepthTest) {
			GL_C(glEnable(GL_DEPTH_TEST));
		}
		else {
			GL_C(glDisable(GL_DEPTH_TEST));
		}

		GL_C(glDepthMask(true));
		GL_C(glDisable(GL_BLEND));
		GL_C(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
		GL_C(glEnable(GL_CULL_FACE));
		GL_C(glFrontFace(GL_CCW));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GL_C(glDepthFunc(GL_LESS));
		
		if (state.mVert.size() == 0) {
			printf("please specify a vertex shader\n");
			exit(1);
		}
		if (state.mFrag.size() == 0) {
			printf("please specify a fragment shader\n");
			exit(1);
		}

		ProgramInfo programInfo = fetchProgram(state.mVert, state.mFrag);

		GL_C(glUseProgram(programInfo.mProgram));

		int iActiveTexture = 0;
		for (const auto& pair : state.mUniforms) {
			std::string uniformName = pair.first;
			UniformValue uniformValue = pair.second;

			if (programInfo.mUniforms.count(uniformName) == 0) {
				continue;
			}
			unsigned int uniformLocation = programInfo.mUniforms[uniformName];

			if (uniformValue.mType == UniformValue::FLOAT_VEC1) {
				GL_C(glUniform1f(uniformLocation, uniformValue.mFloatVec1[0]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC2) {
				GL_C(glUniform2f(uniformLocation, uniformValue.mFloatVec2[0], uniformValue.mFloatVec2[1]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC3) {
				GL_C(glUniform3f(uniformLocation,
					uniformValue.mFloatVec3[0],
					uniformValue.mFloatVec3[1],
					uniformValue.mFloatVec3[2]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC4) {
				GL_C(glUniform4f(uniformLocation,
					uniformValue.mFloatVec4[0],
					uniformValue.mFloatVec3[1],
					uniformValue.mFloatVec4[2],
					uniformValue.mFloatVec4[3]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_MAT4X4) {
				GL_C(glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (GLfloat*)& uniformValue.mFloatMat4x4[0]));
			} else if (uniformValue.mType == UniformValue::TEXTURE2D) {

				GL_C(glUniform1i(uniformLocation, iActiveTexture));
				GL_C(glActiveTexture(GL_TEXTURE0 + iActiveTexture));
				GL_C(glBindTexture(GL_TEXTURE_2D, uniformValue.mTexture2D->mTexture.first));

				++iActiveTexture;
			}
		}
		
		for (const auto& pair : programInfo.mAttributes) {
			std::string attributeName = pair.first;
			unsigned int attributeLocation = pair.second;

			VertexBuffer* attributeVertexBuffer = state.mAttributes[attributeName];

			GLenum type = GL_FLOAT;

			if (!attributeVertexBuffer->mBufferObject.second) {
				printf("forgot to call '.finish()' on the buffer named '%s'\n", attributeVertexBuffer->mName.c_str());
				exit(1);
			}
			
			glBindBuffer(GL_ARRAY_BUFFER, attributeVertexBuffer->mBufferObject.first);
			
			GL_C(glVertexAttribPointer(
				(GLuint)attributeLocation, 
				attributeVertexBuffer->mNumComponents,
				type,
				GL_FALSE, 
				sizeof(float) * attributeVertexBuffer->mNumComponents,
				(void*)0));
			
			GL_C(glEnableVertexAttribArray((GLuint)attributeLocation));
			
		}

		GLenum primitive;
		if (state.mPrimitive == "triangles") {
			primitive = GL_TRIANGLES;
		} else if (state.mPrimitive == "points") {
			primitive = GL_POINTS;
		} else {
			printf("'%s' is an unsupported primitive type\n", state.mPrimitive.c_str());
			exit(1);

		}

		if (state.mIndices != nullptr) {
			if (!state.mIndices->mBufferObject.second) {
				printf("forgot to call '.finish()' on the buffer named '%s'\n", state.mIndices->mName.c_str());
				exit(1);
			}
			
			GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.mIndices->mBufferObject.first));
			GL_C(glDrawElements(primitive, state.mCount, GL_UNSIGNED_INT, 0));

		}
		else {
			// TODO: handle other things than triangles as well.
			GL_C(glDrawArrays(primitive, 0, state.mCount));
		}
	
		//now bind index buffer. and then draw.
		//also, handle rendering without index buffer.


	}
}

void reglCppContext::submit(const Command& command) {

	// current state at top of stack.
	contextState stackState = stateStack.top();

	transferStack(stackState, command);

	submitWithContextState(stackState);
	
}

void reglCppContext::submit(const Command& command, const std::function<void()>& fn) {
	contextState stackState = stateStack.top();
	transferStack(stackState, command);
	stateStack.push(stackState);
	fn();
	stateStack.pop();
}

void reglCppContext::dispose() {
	
	for (auto& pair : programCache) {
		ProgramInfo programCache = pair.second;
		GL_C(glDeleteProgram(programCache.mProgram));
	}
	programCache.clear();

}



}