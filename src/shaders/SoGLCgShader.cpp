#if defined(SO_CG_SHADER_SUPPORT)

#include "SoGLCgShader.h"
#include <iostream>
#include <assert.h>

/***************************************************************************
 ***                   SoGLCgShaderParameter implementation              ***
 ***************************************************************************/

SoGLCgShaderParameter::SoGLCgShaderParameter(SoGLCgShaderObject*   shader,
					     const char*           name,
					     SoGLShader::ValueType type) 
  : SoGLShaderParameter()
{
  this->cgParameter = cgGetNamedParameter(shader->cgProgram, name);
  if (cgIsParameter(this->cgParameter)) {
    CGtype cgType = cgGetParameterType(this->cgParameter);

    this->type = SoGLCgShaderParameter::getParameterTypeFor(cgType);

    if (this->type == type || type == SoGLShader::UNKNOWN_TYPE) {
      this->isReferencedFlag = cgIsParameterReferenced(this->cgParameter);
    }
    else {
      this->isReferencedFlag = FALSE;
      std::cerr << "WARNING in main(): parameter ";
      std::cerr << name;
      std::cerr << " [";
      std::cerr << cgGetTypeString(cgType);
      std::cerr << "]  is of wrong type!" << std::endl;
    }
  }
  else {
    this->type             = SoGLShader::UNKNOWN_TYPE;
    this->isReferencedFlag = FALSE;
  }
}

SoGLShader::ShaderType SoGLCgShaderParameter::shaderType() const
{
  return SoGLShader::CG_SHADER;
}

void SoGLCgShaderParameter::setState(CGGLenum matrix, CGGLenum transform)
{
  if (this->isReferenced() && this->type == SoGLShader::FLOAT_MATRIX4)
    cgGLSetStateMatrixParameter(this->cgParameter, matrix, transform);
}

void SoGLCgShaderParameter::set1f(const float value)
{
  if (this->isReferenced() && isFloat())
    cgGLSetParameter1f(this->cgParameter, value);
}

void SoGLCgShaderParameter::set2f(const float *value)
{
  if (this->isReferenced() && isFloat2())
    cgGLSetParameter2f(this->cgParameter, value[0], value[1]);
}

void SoGLCgShaderParameter::set3f(const float *value)
{
  if (this->isReferenced() && isFloat3())
    cgGLSetParameter3f(this->cgParameter, value[0], value[1], value[2]);
}

void SoGLCgShaderParameter::set4f(const float *value)
{
  if (this->isReferenced() && isFloat4())
    cgGLSetParameter4f(this->cgParameter, value[0], value[1], value[2], value[3]);
}

// --- static methods -------------------------------------------------------

SoGLShader::ValueType SoGLCgShaderParameter::getParameterTypeFor(CGtype type)
{
  switch (type) {
    case CG_FLOAT:
    case CG_FLOAT1:       return SoGLShader::FLOAT;
    case CG_FLOAT2:       return SoGLShader::FLOAT2;
    case CG_FLOAT3:       return SoGLShader::FLOAT3;
    case CG_FLOAT4:       return SoGLShader::FLOAT4;
    case CG_SAMPLER1D:    return SoGLShader::TEXTURE1D;
    case CG_SAMPLER2D:    return SoGLShader::TEXTURE2D;
    case CG_SAMPLER3D:    return SoGLShader::TEXTURE3D;
    case CG_SAMPLERCUBE:  return SoGLShader::TEXTURE_CUBE;
    case CG_SAMPLERRECT:  return SoGLShader::TEXTURE_RECT;
    case CG_FLOAT2x2:     return SoGLShader::FLOAT_MATRIX2;
    case CG_FLOAT3x3:     return SoGLShader::FLOAT_MATRIX3;
    case CG_FLOAT4x4:     return SoGLShader::FLOAT_MATRIX4;
    case CG_UNKNOWN_TYPE: return SoGLShader::UNKNOWN_TYPE;
    default:             
     std::cerr << "cannot map CGtype to ParameterType" << std::endl;
     return SoGLShader::UNKNOWN_TYPE;
  }
}

/***************************************************************************
 ***                    SoGLCgShaderObject implementation                ***
 ***************************************************************************/

CGcontext SoGLCgShaderObject::cgContext     = NULL;
int       SoGLCgShaderObject::instanceCount = 0;

SoGLCgShaderObject::SoGLCgShaderObject()
{
  SoGLCgShaderObject::instanceCount++;
  this->cgProgram = NULL;
}

SoGLCgShaderObject::~SoGLCgShaderObject()
{
  this->unload();
  SoGLCgShaderObject::destroyCgContext();
}

SoGLShader::ShaderType SoGLCgShaderObject::shaderType() const
{
  return SoGLShader::CG_SHADER;
}

SoGLShaderParameter* SoGLCgShaderObject::
getParameter(int index, const char* name, SoGLShader::ValueType type)
{
  return new SoGLCgShaderParameter(this, name, type);
}

SbBool SoGLCgShaderObject::isLoaded() const
{
  return cgIsProgram(this->cgProgram);
}

void SoGLCgShaderObject::load(const char* sourceString)
{
  CGerror errorCode = CG_NO_ERROR;
 
  SoGLCgShaderObject::ensureCgContext();
  
  this->unload();
  
  this->cgProfile = getProfile();
  //cgGLSetOptimalOptions(this->cgProfile);

  cgSetErrorCallback(NULL);
  cgGetError(); // remove last error from stack
  
  this->cgProgram = cgCreateProgram(SoGLCgShaderObject::cgContext,
				CG_SOURCE, 
				sourceString,
				this->cgProfile, 
				"main", // entry's function name
				NULL);  // argument names
  errorCode = cgGetError();

  if (errorCode == CG_NO_ERROR) {
    cgGLLoadProgram(this->cgProgram);
    errorCode = cgGetError();
  }
  
  if (errorCode != CG_NO_ERROR) {
    this->unload();
    SoGLCgShaderObject::printError(errorCode, SoGLCgShaderObject::cgContext);
  }
      
  cgSetErrorCallback(SoGLCgShaderObject::cgErrorCallback);

  return;
}

void SoGLCgShaderObject::unload()
{
  if (cgIsProgram(this->cgProgram)) {
    cgDestroyProgram(this->cgProgram);
    this->cgProgram = NULL;
  }
}


void SoGLCgShaderObject::enable()
{
  if (cgIsProgram(this->cgProgram)) {
    cgGLBindProgram(this->cgProgram);
    cgGLEnableProfile(this->cgProfile);
  }
}

void SoGLCgShaderObject::disable()
{
  if (cgIsProgram(this->cgProgram)) cgGLDisableProfile(this->cgProfile);
}

CGprofile SoGLCgShaderObject::getProfile() const
{
  CGprofile result = isVertexShader() ? CG_PROFILE_ARBVP1 : CG_PROFILE_ARBFP1;

  if (!cgGLIsProfileSupported(result)) {
    std::cerr << "WARNING: profile '" << cgGetProfileString(result) << "'";
    std::cerr << "is not supported." << std::endl;

    if (isVertexShader())
      result = cgGLGetLatestProfile(CG_GL_VERTEX);
    else
      result = cgGLGetLatestProfile(CG_GL_FRAGMENT);

    std::cerr << "'" << cgGetProfileString(result) <<"' will be used instead!";
    std::cerr << std::endl;
  }
  return result;
}

// --- static methods -------------------------------------------------------

void SoGLCgShaderObject::printError(CGerror error, CGcontext context)
{
#if defined(SOURCE_HINT) && 0
    std::cerr << "+++ " << this->sourceHint.getString() << " +++" << std::endl;
#endif
  std::cerr << cgGetErrorString(error)   << std::endl;
  std::cerr << cgGetLastListing(context) << std::endl;
}

void SoGLCgShaderObject::cgErrorCallback()
{
  CGerror lastError = cgGetError();

  if(lastError) printError(lastError, SoGLCgShaderObject::cgContext);
}

void SoGLCgShaderObject::ensureCgContext()
{
  if (!cgIsContext(SoGLCgShaderObject::cgContext)) {
    SoGLCgShaderObject::cgContext = cgCreateContext();
    cgSetErrorCallback(SoGLCgShaderObject::cgErrorCallback);
  }
}

void SoGLCgShaderObject::destroyCgContext()
{
  SoGLCgShaderObject::instanceCount--;

  if (SoGLCgShaderObject::instanceCount > 0) return;
  if (cgIsContext(SoGLCgShaderObject::cgContext)) {
    cgDestroyContext(SoGLCgShaderObject::cgContext);
  }
}


/***************************************************************************
 ***                    SoGLCgShaderObject implementation                ***
 ***************************************************************************/

SoGLCgShaderProgram::SoGLCgShaderProgram()
{
  this->fragmentShader = NULL;
  this->vertexShader   = NULL;
}

SoGLCgShaderProgram::~SoGLCgShaderProgram()
{
}

void SoGLCgShaderProgram::addShaderObject(SoGLCgShaderObject * shaderObject)
{
  if (shaderObject->isVertexShader())
    this->vertexShader = shaderObject;
  else
    this->fragmentShader = shaderObject;
}

void SoGLCgShaderProgram::removeShaderObject(SoGLCgShaderObject * shaderObject)
{
  if (shaderObject == NULL) return;
  
  if (shaderObject->isVertexShader())
    this->vertexShader = NULL;
  else 
    this->fragmentShader = NULL;
}

void SoGLCgShaderProgram::enable()
{
  if (this->fragmentShader) this->fragmentShader->enable();
  if (this->vertexShader)   this->vertexShader->enable();
}

void SoGLCgShaderProgram::disable()
{
  if (this->fragmentShader) this->fragmentShader->disable();
  if (this->vertexShader)   this->vertexShader->disable();
}

#if defined(SOURCE_HINT)
SbString SoGLCgShaderProgram::getSourceHint() const
{
  SbString result;
  
  if (this->fragmentShader && this->fragmentShader->isActive()) {
    SbString str = this->fragmentShader->sourceHint;
    if (str.getLength() > 0) str += " ";
    result += str;
  }
  if (this->vertexShader && this->vertexShader->isActive()) {
    SbString str = this->vertexShader->sourceHint;
    if (str.getLength() > 0) str += " ";
    result += str;
  }
  return result;
}
#endif

#endif /* SO_CG_SHADER_SUPPORT */