/*
  Copyright (c) 2013, Dan VerWeire<dverweire@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <string.h>
#include <napi.h>
#include <uv.h>
#include <node_version.h>
#include <time.h>
#include <uv.h>

#include "odbc.h"
#include "odbc_connection.h"
#include "odbc_result.h"
#include "odbc_statement.h"

using namespace Napi;

Napi::FunctionReference ODBCStatement::constructor;

HENV ODBCStatement::m_hENV;
HDBC ODBCStatement::m_hDBC;
HSTMT ODBCStatement::m_hSTMT;

Napi::Object ODBCStatement::Init(Napi::Env env, Napi::Object exports, HENV hENV, HDBC hDBC, HSTMT hSTMT) {
  DEBUG_PRINTF("ODBCStatement::Init\n");
  Napi::HandleScope scope(env);

  Napi::Function constructorFunction = DefineClass(env, "ODBCStatement", {
    InstanceMethod("execute", &ODBCStatement::Execute),
    InstanceMethod("executeSync", &ODBCStatement::ExecuteSync),
    
    InstanceMethod("executeDirect", &ODBCStatement::ExecuteDirect),
    InstanceMethod("executeDirectSync", &ODBCStatement::ExecuteDirectSync),
    
    InstanceMethod("executeNonQuery", &ODBCStatement::ExecuteNonQuery),
    InstanceMethod("executeNonQuerySync", &ODBCStatement::ExecuteNonQuerySync),
    
    InstanceMethod("prepare", &ODBCStatement::Prepare),
    InstanceMethod("prepareSync", &ODBCStatement::PrepareSync),
    
    InstanceMethod("bind", &ODBCStatement::Bind),
    InstanceMethod("bindSync", &ODBCStatement::BindSync),
    
    InstanceMethod("closeSync", &ODBCStatement::CloseSync)
  });

  // Attach the Database Constructor to the target object
  constructor = Napi::Persistent(constructorFunction);
  constructor.SuppressDestruct();

  ODBCStatement::m_hENV = hENV;
  ODBCStatement::m_hDBC = hDBC;
  ODBCStatement::m_hSTMT = hSTMT;

  exports.Set("ODBCStatement", constructorFunction);

  return exports;

  // ORIGINIAL VVV
  // Napi::FunctionReference t = Napi::Function::New(env, New);

  // // Constructor Template
  
  // t->SetClassName(Napi::String::New(env, "ODBCStatement"));

  // // Reserve space for one Handle<Value>
  // Local<ObjectTemplate> instance_template = t->InstanceTemplate();

  
  // // Prototype Methods
  // InstanceMethod("execute", &Execute),
  // InstanceMethod("executeSync", &ExecuteSync),
  
  // InstanceMethod("executeDirect", &ExecuteDirect),
  // InstanceMethod("executeDirectSync", &ExecuteDirectSync),
  
  // InstanceMethod("executeNonQuery", &ExecuteNonQuery),
  // InstanceMethod("executeNonQuerySync", &ExecuteNonQuerySync),
  
  // InstanceMethod("prepare", &Prepare),
  // InstanceMethod("prepareSync", &PrepareSync),
  
  // InstanceMethod("bind", &Bind),
  // InstanceMethod("bindSync", &BindSync),
  
  // InstanceMethod("closeSync", &CloseSync),

  // // Attach the Database Constructor to the target object
  // constructor.Reset(t->GetFunction());
  // exports.Set(Napi::String::New(env, "ODBCStatement"), t->GetFunction());
}

ODBCStatement::ODBCStatement(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ODBCStatement>(info) {}

ODBCStatement::~ODBCStatement() {
  this->Free();
}

void ODBCStatement::Free() {
  DEBUG_PRINTF("ODBCStatement::Free\n");
  // //if we previously had parameters, then be sure to free them
  if (paramCount) {
    int count = paramCount;
    paramCount = 0;
    
    Parameter prm;
    
    //free parameter memory
    for (int i = 0; i < count; i++) {
      if (prm = params[i], prm.ParameterValuePtr != NULL) {
        switch (prm.ValueType) {
          case SQL_C_WCHAR:   free(prm.ParameterValuePtr);             break;
          case SQL_C_CHAR:    free(prm.ParameterValuePtr);             break; 
          case SQL_C_SBIGINT: delete (int64_t *)prm.ParameterValuePtr; break;
          case SQL_C_DOUBLE:  delete (double  *)prm.ParameterValuePtr; break;
          case SQL_C_BIT:     delete (bool    *)prm.ParameterValuePtr; break;
        }
      }
    }

    free(params);
  }
  
  if (m_hSTMT) {
    uv_mutex_lock(&ODBC::g_odbcMutex);
    
    SQLFreeHandle(SQL_HANDLE_STMT, m_hSTMT);
    m_hSTMT = NULL;
    
    uv_mutex_unlock(&ODBC::g_odbcMutex);
    
    if (bufferLength > 0) {
      free(buffer);
    }
  }
}

Napi::Value ODBCStatement::New(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  DEBUG_PRINTF("ODBCStatement::New\n");
  Napi::HandleScope scope(env);
  
  // REQ_EXT_ARG(0, js_henv);
  // REQ_EXT_ARG(1, js_hdbc);
  // REQ_EXT_ARG(2, js_hstmt);
  
  // HENV hENV = static_cast<HENV>(js_henv->Value());
  // HDBC hDBC = static_cast<HDBC>(js_hdbc->Value());
  // HSTMT hSTMT = static_cast<HSTMT>(js_hstmt->Value());
  
  // //create a new OBCResult object
  // ODBCStatement* stmt = new ODBCStatement(hENV, hDBC, hSTMT);
  
  // //specify the buffer length
  // stmt->bufferLength = MAX_VALUE_SIZE - 1;
  
  // //initialze a buffer for this object
  // stmt->buffer = (uint16_t *) malloc(stmt->bufferLength + 1);
  // //TODO: make sure the malloc succeeded

  // //set the initial colCount to 0
  // stmt->colCount = 0;
  
  // //initialize the paramCount
  // stmt->paramCount = 0;
  
  // stmt->Wrap(info.Holder());
  
  // return info.Holder();
}

/*
 * Execute
 */
class ExecuteAsyncWorker : public Napi::AsyncWorker {

  public:
    ExecuteAsyncWorker(ODBCStatement *odbcStatementObject,  execute_work_data* data, Napi::Function& callback) : Napi::AsyncWorker(callback),
      odbcStatementObject(odbcStatementObject),
      data(data), callback(callback){}

    ~ExecuteAsyncWorker() {}

    void Execute() {
      DEBUG_PRINTF("ODBCStatement::UV_Execute\n");
      // execute_work_data* data = (execute_work_data *)(req->data);
      sqlReturnCode = SQLExecute(data->stmt->m_hSTMT); 
      data->result = sqlReturnCode;
    }

    void OnOK() {
      DEBUG_PRINTF("ODBCStatement::UV_AfterExecute\n");
  
      // execute_work_data* data = (execute_work_data *)(req->data);
      Napi::Env env = Env();
      Napi::HandleScope scope(env);
      
      // //an easy reference to the statment object
      // ODBCStatement* self = data->stmt->self();

      // //First thing, let's check if the execution of the query returned any errors 
      if(data->result == SQL_ERROR) {
        //TODO: Adjust the CallbackSQLERROR Method to return the proper error object.
        // ODBC::CallbackSQLError(env,
        //   SQL_HANDLE_STMT,
        //   odbcStatementObject->m_hSTMT,
        //   callback);

        //For Now jus append a message that error has occured and call the Callback.
        std::string message = "Error Occured During ExecuteAsync() Method";
        std::vector<napi_value> error;
        error.push_back(Napi::String::New(env , message));
        error.push_back(env.Undefined());
        Callback().Call(error);

      }
      // else {
      //   Napi::Value info[4];
      //   bool* canFreeHandle = new bool(false);

      //   info[0] = Napi::External::New(env, self->m_hENV);
      //   info[1] = Napi::External::New(env, self->m_hDBC);
      //   info[2] = Napi::External::New(env, self->m_hSTMT);
      //   info[3] = Napi::External::New(env, canFreeHandle);
        
      //   Napi::Value js_result = Napi::New(env, ODBCResult::constructor)->NewInstance(4, info);

      //   info[0] = env.Null();
      //   info[1] = js_result;

      //   Napi::TryCatch try_catch;

      //   data->cb->Call(2, info);

      //   if (try_catch.HasCaught()) {
      //       Napi::FatalException(try_catch);
      //   }
      // }

      // self->Unref();
      // delete data->cb;
      
      // free(data);
      // free(req);
    }

  private:
    ODBCStatement *odbcStatementObject;
    execute_work_data* data;
    SQLRETURN sqlReturnCode;
    Napi::Function& callback;
};

Napi::Value ODBCStatement::Execute(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::Execute\n");
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  //Check to Ensure that the first parameter is a callback function.
  REQ_FUN_ARG(0, info);

  //ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
  // uv_work_t* work_req = (uv_work_t *) (calloc(1, sizeof(uv_work_t)));
  
  execute_work_data* data = 
    (execute_work_data *) calloc(1, sizeof(execute_work_data));

  // data->cb = new Napi::FunctionReference(info[0]);
  Napi::Function callback = info[0].As<Napi::Function>();
  
  data->stmt = this;
  // work_req->data = data;
  
  // uv_queue_work(
  //   uv_default_loop(),
  //   work_req,
  //   UV_Execute,
  //   (uv_after_work_cb)UV_AfterExecute);

  // stmt->Ref();

  ExecuteAsyncWorker *worker = new ExecuteAsyncWorker(this, data, callback);
  worker->Queue();

  return env.Undefined();
}



void ODBCStatement::UV_Execute(uv_work_t* req) {
  // DEBUG_PRINTF("ODBCStatement::UV_Execute\n");
  
  // execute_work_data* data = (execute_work_data *)(req->data);

  // SQLRETURN ret;
  
  // ret = SQLExecute(data->stmt->m_hSTMT); 

  // data->result = ret;
}

void ODBCStatement::UV_AfterExecute(uv_work_t* req, int status) {
  // DEBUG_PRINTF("ODBCStatement::UV_AfterExecute\n");
  
  // execute_work_data* data = (execute_work_data *)(req->data);
  
  // Napi::HandleScope scope(env);
  
  // //an easy reference to the statment object
  // ODBCStatement* self = data->stmt->self();

  // //First thing, let's check if the execution of the query returned any errors 
  // if(data->result == SQL_ERROR) {
  //   ODBC::CallbackSQLError(
  //     SQL_HANDLE_STMT,
  //     self->m_hSTMT,
  //     data->cb);
  // }
  // else {
  //   Napi::Value info[4];
  //   bool* canFreeHandle = new bool(false);

  //   info[0] = Napi::External::New(env, self->m_hENV);
  //   info[1] = Napi::External::New(env, self->m_hDBC);
  //   info[2] = Napi::External::New(env, self->m_hSTMT);
  //   info[3] = Napi::External::New(env, canFreeHandle);
    
  //   Napi::Value js_result = Napi::New(env, ODBCResult::constructor)->NewInstance(4, info);

  //   info[0] = env.Null();
  //   info[1] = js_result;

  //   Napi::TryCatch try_catch;

  //   data->cb->Call(2, info);

  //   if (try_catch.HasCaught()) {
  //       Napi::FatalException(try_catch);
  //   }
  // }

  // self->Unref();
  // delete data->cb;
  
  // free(data);
  // free(req);
}

/*
 * ExecuteSync
 * 
 */

Napi::Value ODBCStatement::ExecuteSync(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::ExecuteSync\n");
  
  // Napi::HandleScope scope(env);

  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();

  // SQLRETURN ret = SQLExecute(stmt->m_hSTMT); 
  
  // if(ret == SQL_ERROR) {
  //   Napi::Value objError = ODBC::GetSQLError(
  //     SQL_HANDLE_STMT,
  //     stmt->m_hSTMT,
  //     (char *) "[node-odbc] Error in ODBCStatement::ExecuteSync"
  //   );

  //   Napi::Error::New(env, objError).ThrowAsJavaScriptException();

    
  //   return env.Null();
  // }
  // else {
  //   Napi::Value result[4];
  //   bool* canFreeHandle = new bool(false);
    
  //   result[0] = Napi::External::New(env, stmt->m_hENV);
  //   result[1] = Napi::External::New(env, stmt->m_hDBC);
  //   result[2] = Napi::External::New(env, stmt->m_hSTMT);
  //   result[3] = Napi::External::New(env, canFreeHandle);
    
  //   Napi::Object js_result = Napi::New(env, ODBCResult::constructor)->NewInstance(4, result);

  //   return js_result;
  // }
}

/*
 * ExecuteNonQuery
 */

class ExecuteNonQueryAsyncWorker : public Napi::AsyncWorker {

  public:
    ExecuteNonQueryAsyncWorker(ODBCStatement *odbcStatementObject,  execute_work_data* data, Napi::Function& callback) : Napi::AsyncWorker(callback),
      odbcStatementObject(odbcStatementObject),
      data(data), callback(callback){}

    ~ExecuteNonQueryAsyncWorker() {}

    void Execute() {
      DEBUG_PRINTF("ODBCStatement::ExecuteNonQuery\n");
      // execute_work_data* data = (execute_work_data *)(req->data);
      sqlReturnCode = SQLExecute(data->stmt->m_hSTMT); 
      data->result = sqlReturnCode;
    }

    void OnOK() {
      // DEBUG_PRINTF("ODBCStatement::ExecuteNonQuery\n");
  
      // execute_work_data* data = (execute_work_data *)(req->data);
      Napi::Env env = Env();      
      Napi::HandleScope scope(env);
      
      //an easy reference to the statment object
      // ODBCStatement* self = data->stmt->self();

      //First thing, let's check if the execution of the query returned any errors 
      if(data->result == SQL_ERROR) {
         //TODO: Adjust the CallbackSQLERROR Method to return the proper error object.
        // ODBC::CallbackSQLError(env,
        //   SQL_HANDLE_STMT,
        //   odbcStatementObject->m_hSTMT,
        //   callback);

        //For Now jus append a message that error has occured and call the Callback.
        std::string message = "Error Occured During ExecuteNonQueryAsync() Method";
        std::vector<napi_value> error;
        error.push_back(Napi::String::New(env , message));
        error.push_back(env.Undefined());
        Callback().Call(error);
      }
      // else {
      //   SQLLEN rowCount = 0;
        
      //   SQLRETURN ret = SQLRowCount(odbcStatementObject->m_hSTMT, &rowCount);
        
      //   if (!SQL_SUCCEEDED(ret)) {
      //     rowCount = 0;
      //   }
        
      //   uv_mutex_lock(&ODBC::g_odbcMutex);
      //   SQLFreeStmt(self->m_hSTMT, SQL_CLOSE);
      //   uv_mutex_unlock(&ODBC::g_odbcMutex);
        
      //   Napi::Value info[2];

      //   info[0] = env.Null();
      //   // We get a potential loss of precision here. Number isn't as big as int64. Probably fine though.
      //   info[1] = Napi::Number::New(env, rowCount);

      //   Napi::TryCatch try_catch;
        
      //   data->cb->Call(2, info);

      //   if (try_catch.HasCaught()) {
      //       Napi::FatalException(try_catch);
      //   }
      // }

      // self->Unref();
      // delete data->cb;
      
      // free(data);
      // free(req);
  
    }

  private:
    ODBCStatement *odbcStatementObject;
    execute_work_data* data;
    SQLRETURN sqlReturnCode;
    Napi::Function& callback;
};

Napi::Value ODBCStatement::ExecuteNonQuery(const Napi::CallbackInfo& info) {
  DEBUG_PRINTF("ODBCStatement::ExecuteNonQuery\n");
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  REQ_FUN_ARG(0, info);

  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
  // uv_work_t* work_req = (uv_work_t *) (calloc(1, sizeof(uv_work_t)));
  
  execute_work_data* data = 
    (execute_work_data *) calloc(1, sizeof(execute_work_data));

  // data->cb = new Napi::FunctionReference(cb);
  Napi::Function callback = info[0].As<Napi::Function>();
  data->stmt = this;
  // work_req->data = data;
  
  // uv_queue_work(
  //   uv_default_loop(),
  //   work_req,
  //   UV_ExecuteNonQuery,
  //   (uv_after_work_cb)UV_AfterExecuteNonQuery);

  // stmt->Ref();
  
  // return env.Undefined();
  ExecuteNonQueryAsyncWorker *worker = new ExecuteNonQueryAsyncWorker(this, data, callback);
  worker->Queue();

  return env.Undefined();
}

void ODBCStatement::UV_ExecuteNonQuery(uv_work_t* req) {
  // DEBUG_PRINTF("ODBCStatement::ExecuteNonQuery\n");
  
  // execute_work_data* data = (execute_work_data *)(req->data);

  // SQLRETURN ret;
  
  // ret = SQLExecute(data->stmt->m_hSTMT); 

  // data->result = ret;
}

void ODBCStatement::UV_AfterExecuteNonQuery(uv_work_t* req, int status) {
  // DEBUG_PRINTF("ODBCStatement::ExecuteNonQuery\n");
  
  // execute_work_data* data = (execute_work_data *)(req->data);
  
  // Napi::HandleScope scope(env);
  
  // //an easy reference to the statment object
  // ODBCStatement* self = data->stmt->self();

  // //First thing, let's check if the execution of the query returned any errors 
  // if(data->result == SQL_ERROR) {
  //   ODBC::CallbackSQLError(
  //     SQL_HANDLE_STMTi,
  //     self->m_hSTMTi,
  //     data->cb);
  // }
  // else {
  //   SQLLEN rowCount = 0;
    
  //   SQLRETURN ret = SQLRowCount(self->m_hSTMT, &rowCount);
    
  //   if (!SQL_SUCCEEDED(ret)) {
  //     rowCount = 0;
  //   }
    
  //   uv_mutex_lock(&ODBC::g_odbcMutex);
  //   SQLFreeStmt(self->m_hSTMT, SQL_CLOSE);
  //   uv_mutex_unlock(&ODBC::g_odbcMutex);
    
  //   Napi::Value info[2];

  //   info[0] = env.Null();
  //   // We get a potential loss of precision here. Number isn't as big as int64. Probably fine though.
  //   info[1] = Napi::Number::New(env, rowCount);

  //   Napi::TryCatch try_catch;
    
  //   data->cb->Call(2, info);

  //   if (try_catch.HasCaught()) {
  //       Napi::FatalException(try_catch);
  //   }
  // }

  // self->Unref();
  // delete data->cb;
  
  // free(data);
  // free(req);
}

/*
 * ExecuteNonQuerySync
 * 
 */

Napi::Value ODBCStatement::ExecuteNonQuerySync(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::ExecuteNonQuerySync\n");
  
  // Napi::HandleScope scope(env);

  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();

  // SQLRETURN ret = SQLExecute(stmt->m_hSTMT); 
  
  // if(ret == SQL_ERROR) {
  //   Napi::Value objError = ODBC::GetSQLError(
  //     SQL_HANDLE_STMT,
  //     stmt->m_hSTMT,
  //     (char *) "[node-odbc] Error in ODBCStatement::ExecuteSync"
  //   );

  //   Napi::Error::New(env, objError).ThrowAsJavaScriptException();

    
  //   return env.Null();
  // }
  // else {
  //   SQLLEN rowCount = 0;
    
  //   SQLRETURN ret = SQLRowCount(stmt->m_hSTMT, &rowCount);
    
  //   if (!SQL_SUCCEEDED(ret)) {
  //     rowCount = 0;
  //   }
    
  //   uv_mutex_lock(&ODBC::g_odbcMutex);
  //   SQLFreeStmt(stmt->m_hSTMT, SQL_CLOSE);
  //   uv_mutex_unlock(&ODBC::g_odbcMutex);
    
  //   return Napi::Number::New(env, rowCount);
  // }
}

/*
 * ExecuteDirect
 * 
 */

class ExecuteDirectAsyncWorker : public Napi::AsyncWorker {

  public:
    ExecuteDirectAsyncWorker(ODBCStatement *odbcStatementObject,  execute_work_data* data, Napi::Function& callback) : Napi::AsyncWorker(callback),
      odbcStatementObject(odbcStatementObject),
      data(data), callback(callback){}

    ~ExecuteDirectAsyncWorker() {}

    void Execute() {
      
    }

    void OnOK() {
  
    }

  private:
    ODBCStatement *odbcStatementObject;
    execute_work_data* data;
    SQLRETURN sqlReturnCode;
    Napi::Function& callback;
};

Napi::Value ODBCStatement::ExecuteDirect(const Napi::CallbackInfo& info) {
//   DEBUG_PRINTF("ODBCStatement::ExecuteDirect\n");
  
//   Napi::HandleScope scope(env);

//   REQ_STRO_ARG(0, sql);
//   REQ_FUN_ARG(1, cb);

//   ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
//   uv_work_t* work_req = (uv_work_t *) (calloc(1, sizeof(uv_work_t)));
  
//   execute_direct_work_data* data = 
//     (execute_direct_work_data *) calloc(1, sizeof(execute_direct_work_data));

//   data->cb = new Napi::FunctionReference(cb);

// #ifdef UNICODE
//   data->sqlLen = sql->Length();
//   data->sql = (uint16_t *) malloc((data->sqlLen * sizeof(uint16_t)) + sizeof(uint16_t));
//   sql->Write((uint16_t *) data->sql);
// #else
//   data->sqlLen = sql->Utf8Length();
//   data->sql = (char *) malloc(data->sqlLen +1);
//   sql->WriteUtf8((char *) data->sql);
// #endif

//   data->stmt = stmt;
//   work_req->data = data;
  
//   uv_queue_work(
//     uv_default_loop(),
//     work_req, 
//     UV_ExecuteDirect, 
//     (uv_after_work_cb)UV_AfterExecuteDirect);

//   stmt->Ref();

//   return env.Undefined();
}

void ODBCStatement::UV_ExecuteDirect(uv_work_t* req) {
  // DEBUG_PRINTF("ODBCStatement::UV_ExecuteDirect\n");
  
  // execute_direct_work_data* data = (execute_direct_work_data *)(req->data);

  // SQLRETURN ret;
  
  // ret = SQLExecDirect(
  //   data->stmt->m_hSTMT,
  //   (SQLTCHAR *) data->sql, 
  //   data->sqlLen);  

  // data->result = ret;
}

void ODBCStatement::UV_AfterExecuteDirect(uv_work_t* req, int status) {
  // DEBUG_PRINTF("ODBCStatement::UV_AfterExecuteDirect\n");
  
  // execute_direct_work_data* data = (execute_direct_work_data *)(req->data);
  
  // Napi::HandleScope scope(env);
  
  // //an easy reference to the statment object
  // ODBCStatement* self = data->stmt->self();

  // //First thing, let's check if the execution of the query returned any errors 
  // if(data->result == SQL_ERROR) {
  //   ODBC::CallbackSQLError(
  //     SQL_HANDLE_STMT,
  //     self->m_hSTMT,
  //     data->cb);
  // }
  // else {
  //   Napi::Value info[4];
  //   bool* canFreeHandle = new bool(false);
    
  //   info[0] = Napi::External::New(env, self->m_hENV);
  //   info[1] = Napi::External::New(env, self->m_hDBC);
  //   info[2] = Napi::External::New(env, self->m_hSTMT);
  //   info[3] = Napi::External::New(env, canFreeHandle);
    
  //   Napi::Object js_result =  Napi::Function::New(env, ODBCResult::constructor)->NewInstance(4, info);

  //   info[0] = env.Null();
  //   info[1] = js_result;

  //   Napi::TryCatch try_catch;

  //   data->cb->Call(2, info);

  //   if (try_catch.HasCaught()) {
  //       Napi::FatalException(try_catch);
  //   }
  // }

  // self->Unref();
  // delete data->cb;
  
  // free(data->sql);
  // free(data);
  // free(req);
}

/*
 * ExecuteDirectSync
 * 
 */

Napi::Value ODBCStatement::ExecuteDirectSync(const Napi::CallbackInfo& info) {
//   DEBUG_PRINTF("ODBCStatement::ExecuteDirectSync\n");
  
//   Napi::HandleScope scope(env);

// #ifdef UNICODE
//   REQ_WSTR_ARG(0, sql);
// #else
//   REQ_STR_ARG(0, sql);
// #endif

//   ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
//   SQLRETURN ret = SQLExecDirect(
//     stmt->m_hSTMT,
//     (SQLTCHAR *) *sql, 
//     sql.Length());  

//   if(ret == SQL_ERROR) {
//     Napi::Value objError = ODBC::GetSQLError(
//       SQL_HANDLE_STMT,
//       stmt->m_hSTMT,
//       (char *) "[node-odbc] Error in ODBCStatement::ExecuteDirectSync"
//     );

//     Napi::Error::New(env, objError).ThrowAsJavaScriptException();

    
//     return env.Null();
//   }
//   else {
//     Napi::Value result[4];
//     bool* canFreeHandle = new bool(false);
    
//     result[0] = Napi::External::New(env, stmt->m_hENV);
//     result[1] = Napi::External::New(env, stmt->m_hDBC);
//     result[2] = Napi::External::New(env, stmt->m_hSTMT);
//     result[3] = Napi::External::New(env, canFreeHandle);
    
//     Napi::Object js_result = Napi::Function::New(env, ODBCResult::constructor)->NewInstance(4, result);
    
//     return js_result;
//   }
}

/*
 * PrepareSync
 * 
 */

Napi::Value ODBCStatement::PrepareSync(const Napi::CallbackInfo& info) {
//   DEBUG_PRINTF("ODBCStatement::PrepareSync\n");
  
//   Napi::HandleScope scope(env);

//   REQ_STRO_ARG(0, sql);

//   ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();

//   SQLRETURN ret;

// #ifdef UNICODE
//   int sqlLen = sql->Length() + 1;
//   uint16_t* sql2 = (uint16_t *) malloc(sqlLen * sizeof(uint16_t));
//   sql->Write(sql2);
// #else
//   int sqlLen = sql->Utf8Length() + 1;
//   char* sql2 = (char *) malloc(sqlLen);
//   sql->WriteUtf8(sql2);
// #endif
  
//   ret = SQLPrepare(
//     stmt->m_hSTMT,
//     (SQLTCHAR *) sql2, 
//     sqlLen);
  
//   if (SQL_SUCCEEDED(ret)) {
//     return env.True();
//   }
//   else {
//     Napi::Value objError = ODBC::GetSQLError(
//       SQL_HANDLE_STMT,
//       stmt->m_hSTMT,
//       (char *) "[node-odbc] Error in ODBCStatement::PrepareSync"
//     );

//     Napi::Error::New(env, objError).ThrowAsJavaScriptException();


//     return env.False();
//   }
}

/*
 * Prepare
 * 
 */
class PrepareAsyncWorker : public Napi::AsyncWorker {

  public:
    PrepareAsyncWorker(ODBCStatement *odbcStatementObject,  execute_work_data* data, Napi::Function& callback) : Napi::AsyncWorker(callback),
      odbcStatementObject(odbcStatementObject),
      data(data), callback(callback){}

    ~PrepareAsyncWorker() {}

    void Execute() {
      
    }

    void OnOK() {
  
    }

  private:
    ODBCStatement *odbcStatementObject;
    execute_work_data* data;
    SQLRETURN sqlReturnCode;
    Napi::Function& callback;
};

Napi::Value ODBCStatement::Prepare(const Napi::CallbackInfo& info) {
//   DEBUG_PRINTF("ODBCStatement::Prepare\n");
  
//   Napi::HandleScope scope(env);

//   REQ_STRO_ARG(0, sql);
//   REQ_FUN_ARG(1, cb);

//   ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
//   uv_work_t* work_req = (uv_work_t *) (calloc(1, sizeof(uv_work_t)));
  
//   prepare_work_data* data = 
//     (prepare_work_data *) calloc(1, sizeof(prepare_work_data));

//   data->cb = new Napi::FunctionReference(cb);

// #ifdef UNICODE
//   data->sqlLen = sql->Length();
//   data->sql = (uint16_t *) malloc((data->sqlLen * sizeof(uint16_t)) + sizeof(uint16_t));
//   sql->Write((uint16_t *) data->sql);
// #else
//   data->sqlLen = sql->Utf8Length();
//   data->sql = (char *) malloc(data->sqlLen +1);
//   sql->WriteUtf8((char *) data->sql);
// #endif
  
//   data->stmt = stmt;
  
//   work_req->data = data;
  
//   uv_queue_work(
//     uv_default_loop(), 
//     work_req, 
//     UV_Prepare, 
//     (uv_after_work_cb)UV_AfterPrepare);

//   stmt->Ref();

//   return env.Undefined();
}

void ODBCStatement::UV_Prepare(uv_work_t* req) {
  // DEBUG_PRINTF("ODBCStatement::UV_Prepare\n");
  
  // prepare_work_data* data = (prepare_work_data *)(req->data);

  // DEBUG_PRINTF("ODBCStatement::UV_Prepare\n");
  // //DEBUG_PRINTF("ODBCStatement::UV_Prepare m_hDBC=%X m_hDBC=%X m_hSTMT=%X\n",
  // //  data->stmt->m_hENV,
  // //  data->stmt->m_hDBC,
  // //  data->stmt->m_hSTMT
  // //);
  
  // SQLRETURN ret;
  
  // ret = SQLPrepare(
  //   data->stmt->m_hSTMT,
  //   (SQLTCHAR *) data->sql, 
  //   data->sqlLen);

  // data->result = ret;
}

void ODBCStatement::UV_AfterPrepare(uv_work_t* req, int status) {
  // DEBUG_PRINTF("ODBCStatement::UV_AfterPrepare\n");
  
  // prepare_work_data* data = (prepare_work_data *)(req->data);
  
  // DEBUG_PRINTF("ODBCStatement::UV_AfterPrepare\n");
  // //DEBUG_PRINTF("ODBCStatement::UV_AfterPrepare m_hDBC=%X m_hDBC=%X m_hSTMT=%X\n",
  // //  data->stmt->m_hENV,
  // //  data->stmt->m_hDBC,
  // //  data->stmt->m_hSTMT
  // //);
  
  // Napi::HandleScope scope(env);

  // //First thing, let's check if the execution of the query returned any errors 
  // if(data->result == SQL_ERROR) {
  //   ODBC::CallbackSQLError(
  //     SQL_HANDLE_STMT,
  //     data->stmt->m_hSTMT,
  //     data->cb);
  // }
  // else {
  //   Napi::Value info[2];

  //   info[0] = env.Null();
  //   info[1] = env.True();

  //   Napi::TryCatch try_catch;

  //   data->cb->Call( 2, info);

  //   if (try_catch.HasCaught()) {
  //       Napi::FatalException(try_catch);
  //   }
  // }
  
  // data->stmt->Unref();
  // delete data->cb;
  
  // free(data->sql);
  // free(data);
  // free(req);
}

/*
 * BindSync
 * 
 */

Napi::Value ODBCStatement::BindSync(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::BindSync\n");
  
  // Napi::HandleScope scope(env);

  // if ( !info[0].IsArray() ) {
  //   Napi::TypeError::New(env, "Argument 1 must be an Array").ThrowAsJavaScriptException();
  //   return env.Null();
  // }

  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
  // DEBUG_PRINTF("ODBCStatement::BindSync\n");
  // //DEBUG_PRINTF("ODBCStatement::BindSync m_hDBC=%X m_hDBC=%X m_hSTMT=%X\n",
  // //  stmt->m_hENV,
  // //  stmt->m_hDBC,
  // //  stmt->m_hSTMT
  // //);
  
  // //if we previously had parameters, then be sure to free them
  // //before allocating more
  // if (stmt->paramCount) {
  //   int count = stmt->paramCount;
  //   stmt->paramCount = 0;
    
  //   Parameter prm;
    
  //   //free parameter memory
  //   for (int i = 0; i < count; i++) {
  //     if (prm = stmt->params[i], prm.ParameterValuePtr != NULL) {
  //       switch (prm.ValueType) {
  //         case SQL_C_WCHAR:   free(prm.ParameterValuePtr);             break;
  //         case SQL_C_CHAR:    free(prm.ParameterValuePtr);             break; 
  //         case SQL_C_SBIGINT: delete (int64_t *)prm.ParameterValuePtr; break;
  //         case SQL_C_DOUBLE:  delete (double  *)prm.ParameterValuePtr; break;
  //         case SQL_C_BIT:     delete (bool    *)prm.ParameterValuePtr; break;
  //       }
  //     }
  //   }

  //   free(stmt->params);
  // }
  
  // stmt->params = ODBC::GetParametersFromArray(
  //   info[0].As<Napi::Array>(), 
  //   &stmt->paramCount);
  
  // SQLRETURN ret = SQL_SUCCESS;
  // Parameter prm;
  
  // for (int i = 0; i < stmt->paramCount; i++) {
  //   prm = stmt->params[i];
    
  //   /*DEBUG_PRINTF(
  //     "ODBCStatement::BindSync - param[%i]: c_type=%i type=%i "
  //     "buffer_length=%i size=%i length=%i &length=%X decimals=%i value=%s\n",
  //     i, prm.ValueType, prm.ParameterType, prm.BufferLength, prm.ColumnSize, prm.length, 
  //     &stmt->params[i].StrLen_or_IndPtr, prm.DecimalDigits, prm.ParameterValuePtr
  //   );*/

  //   ret = SQLBindParameter(
  //     stmt->m_hSTMT,      //StatementHandle
  //     i + 1,              //ParameterNumber
  //     SQL_PARAM_INPUT,    //InputOutputType
  //     prm.ValueType,
  //     prm.ParameterType,
  //     prm.ColumnSize,
  //     prm.DecimalDigits,
  //     prm.ParameterValuePtr,
  //     prm.BufferLength,
  //     &stmt->params[i].StrLen_or_IndPtr);

  //   if (ret == SQL_ERROR) {
  //     break;
  //   }
  // }

  // if (SQL_SUCCEEDED(ret)) {
  //   return env.True();
  // }
  // else {
  //   Napi::Value objError = ODBC::GetSQLError(
  //     SQL_HANDLE_STMT,
  //     stmt->m_hSTMT,
  //     (char *) "[node-odbc] Error in ODBCStatement::BindSync"
  //   );

  //   Napi::Error::New(env, objError).ThrowAsJavaScriptException();

    
  //   return env.False();
  // }
}

/*
 * Bind
 * 
 */
class BindAsyncWorker : public Napi::AsyncWorker {

  public:
    BindAsyncWorker(ODBCStatement *odbcStatementObject,  execute_work_data* data, Napi::Function& callback) : Napi::AsyncWorker(callback),
      odbcStatementObject(odbcStatementObject),
      data(data), callback(callback){}

    ~BindAsyncWorker() {}

    void Execute() {
      
    }

    void OnOK() {
  
    }

  private:
    ODBCStatement *odbcStatementObject;
    execute_work_data* data;
    SQLRETURN sqlReturnCode;
    Napi::Function& callback;
};

Napi::Value ODBCStatement::Bind(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::Bind\n");
  
  // Napi::HandleScope scope(env);

  // if ( !info[0].IsArray() ) {
  //   Napi::Error::New(env, "Argument 1 must be an Array").ThrowAsJavaScriptException();
  //   return env.Null();
  // }
  
  // REQ_FUN_ARG(1, cb);

  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
  // uv_work_t* work_req = (uv_work_t *) (calloc(1, sizeof(uv_work_t)));
  
  // bind_work_data* data = 
  //   (bind_work_data *) calloc(1, sizeof(bind_work_data));

  // //if we previously had parameters, then be sure to free them
  // //before allocating more
  // if (stmt->paramCount) {
  //   int count = stmt->paramCount;
  //   stmt->paramCount = 0;
    
  //   Parameter prm;
    
  //   //free parameter memory
  //   for (int i = 0; i < count; i++) {
  //     if (prm = stmt->params[i], prm.ParameterValuePtr != NULL) {
  //       switch (prm.ValueType) {
  //         case SQL_C_WCHAR:   free(prm.ParameterValuePtr);             break;
  //         case SQL_C_CHAR:    free(prm.ParameterValuePtr);             break; 
  //         case SQL_C_SBIGINT: delete (int64_t *)prm.ParameterValuePtr; break;
  //         case SQL_C_DOUBLE:  delete (double  *)prm.ParameterValuePtr; break;
  //         case SQL_C_BIT:     delete (bool    *)prm.ParameterValuePtr; break;
  //       }
  //     }
  //   }

  //   free(stmt->params);
  // }
  
  // data->stmt = stmt;
  
  // DEBUG_PRINTF("ODBCStatement::Bind\n");
  // //DEBUG_PRINTF("ODBCStatement::Bind m_hDBC=%X m_hDBC=%X m_hSTMT=%X\n",
  // //  data->stmt->m_hENV,
  // //  data->stmt->m_hDBC,
  // //  data->stmt->m_hSTMT
  // //);
  
  // data->cb = new Napi::FunctionReference(cb);
  
  // data->stmt->params = ODBC::GetParametersFromArray(
  //   info[0].As<Napi::Array>(), 
  //   &data->stmt->paramCount);
  
  // work_req->data = data;
  
  // uv_queue_work(
  //   uv_default_loop(), 
  //   work_req, 
  //   UV_Bind, 
  //   (uv_after_work_cb)UV_AfterBind);

  // stmt->Ref();

  // return env.Undefined();
}

void ODBCStatement::UV_Bind(uv_work_t* req) {
  // DEBUG_PRINTF("ODBCStatement::UV_Bind\n");
  
  // bind_work_data* data = (bind_work_data *)(req->data);

  // DEBUG_PRINTF("ODBCStatement::UV_Bind\n");
  // //DEBUG_PRINTF("ODBCStatement::UV_Bind m_hDBC=%X m_hDBC=%X m_hSTMT=%X\n",
  // //  data->stmt->m_hENV,
  // //  data->stmt->m_hDBC,
  // //  data->stmt->m_hSTMT
  // //);
  
  // SQLRETURN ret = SQL_SUCCESS;
  // Parameter prm;
  
  // for (int i = 0; i < data->stmt->paramCount; i++) {
  //   prm = data->stmt->params[i];
    
  //   /*DEBUG_PRINTF(
  //     "ODBCStatement::UV_Bind - param[%i]: c_type=%i type=%i "
  //     "buffer_length=%i size=%i length=%i &length=%X decimals=%i value=%s\n",
  //     i, prm.ValueType, prm.ParameterType, prm.BufferLength, prm.ColumnSize, prm.length, 
  //     &data->stmt->params[i].StrLen_or_IndPtr, prm.DecimalDigits, prm.ParameterValuePtr
  //   );*/

  //   ret = SQLBindParameter(
  //     data->stmt->m_hSTMT,        //StatementHandle
  //     i + 1,                      //ParameterNumber
  //     SQL_PARAM_INPUT,            //InputOutputType
  //     prm.ValueType,
  //     prm.ParameterType,
  //     prm.ColumnSize,
  //     prm.DecimalDigits,
  //     prm.ParameterValuePtr,
  //     prm.BufferLength,
  //     &data->stmt->params[i].StrLen_or_IndPtr);

  //   if (ret == SQL_ERROR) {
  //     break;
  //   }
  // }

  // data->result = ret;
}

void ODBCStatement::UV_AfterBind(uv_work_t* req, int status) {
  // DEBUG_PRINTF("ODBCStatement::UV_AfterBind\n");
  
  // bind_work_data* data = (bind_work_data *)(req->data);
  
  // Napi::HandleScope scope(env);
  
  // //an easy reference to the statment object
  // ODBCStatement* self = data->stmt->self();

  // //Check if there were errors 
  // if(data->result == SQL_ERROR) {
  //   ODBC::CallbackSQLError(
  //     SQL_HANDLE_STMT,
  //     self->m_hSTMT,
  //     data->cb);
  // }
  // else {
  //   Napi::Value info[2];

  //   info[0] = env.Null();
  //   info[1] = env.True();

  //   Napi::TryCatch try_catch;

  //   data->cb->Call( 2, info);

  //   if (try_catch.HasCaught()) {
  //       Napi::FatalException(try_catch);
  //   }
  // }

  // self->Unref();
  // delete data->cb;
  
  // free(data);
  // free(req);
}

/*
 * CloseSync
 */

Napi::Value ODBCStatement::CloseSync(const Napi::CallbackInfo& info) {
  // DEBUG_PRINTF("ODBCStatement::CloseSync\n");
  
  // Napi::HandleScope scope(env);

  // OPT_INT_ARG(0, closeOption, SQL_DESTROY);
  
  // ODBCStatement* stmt = info.Holder().Unwrap<ODBCStatement>();
  
  // DEBUG_PRINTF("ODBCStatement::CloseSync closeOption=%i\n", 
  //              closeOption);
  
  // if (closeOption == SQL_DESTROY) {
  //   stmt->Free();
  // }
  // else {
  //   uv_mutex_lock(&ODBC::g_odbcMutex);
    
  //   SQLFreeStmt(stmt->m_hSTMT, closeOption);
  
  //   uv_mutex_unlock(&ODBC::g_odbcMutex);
  // }

  // return env.True();
}
