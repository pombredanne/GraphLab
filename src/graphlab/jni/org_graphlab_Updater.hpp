/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

/**
 * @file org_graphlab_Updater.hpp
 * 
 * @author Jiunn Haur Lim <jiunnhal@cmu.edu>
 */

#ifndef ORG_GRAPHLAB_UPDATER_HPP
#define ORG_GRAPHLAB_UPDATER_HPP

#include <graphlab.hpp>
#include "org_graphlab_Core.hpp"
#include "org_graphlab_Updater.h"

/** Proxy edge */
struct proxy_edge {};

/**
 * Proxy vertex.
 * Contains the vertex ID of the corresponding application vertex.
 */
struct proxy_vertex {
  /** corresponding application vertex ID */
  int app_id;
};

/** Proxy graph */
typedef graphlab::graph<proxy_vertex, proxy_edge> proxy_graph;

/**
 * Proxy updater.
 * Mirrors and forwards update calls to the corresponding Java updater.
 * This object should be created each time a org.graphlab.Updater object is
 * created; it creates a new reference to the Java object (so that it doesn't
 * get garbage collected.) The destructor will correspondingly delete the
 * reference to allow the corresponding Java object to be garbaged collected.
 */
class proxy_updater : 
  public graphlab::iupdate_functor<proxy_graph, proxy_updater> {
  
private:
  
  /** org.graphlab.Updater object */
  jobject mjava_updater;
  
public:

  /**
   * Method ID of org.graphlab.Updater#execUpdate.
   * Set this once per JVM.
   */
  static jmethodID java_method_id;
  
  /**
   * Constructor for proxy updater.
   * Initializes this object with the associated Java org.graphlab.Updater
   * object.
   * @param[in] env           JNI environment - used to create a new reference
   *                          to javaUpdater.
   * @param[in] java_updater  Java org.graphlab.Updater object. This constructor
   *                          will create a new reference to the object to prevent
   *                          garbage collection.
   */
  proxy_updater(JNIEnv *env, jobject &java_updater){
    mjava_updater = env->NewGlobalRef(java_updater);
  }
  
  /** The default constructor does nothing */
  proxy_updater() : mjava_updater(NULL){}
  
  proxy_updater(const proxy_updater& other){
  
    if (NULL == other.mjava_updater){
      this->mjava_updater = NULL;
      return;
    }
    
    // create another reference
    JNIEnv *env = graphlab::jni_core<proxy_graph, proxy_updater>::get_jni_env();
    this->mjava_updater = env->NewGlobalRef(other.mjava_updater);
    
  }
  
  proxy_updater & operator=(const proxy_updater &other){
  
    if (this != &other){
    
      JNIEnv *env = graphlab::jni_core<proxy_graph, proxy_updater>::get_jni_env();
      jobject java_updater = NULL;
      
      // if other has a java object, create a new ref
      if (NULL != other.mjava_updater)
        java_updater = env->NewGlobalRef(other.mjava_updater);
      
      // if this has a java object, delete ref
      if (NULL != this->mjava_updater)
        env->DeleteGlobalRef(this->mjava_updater);
        
      // assign!
      this->mjava_updater = java_updater;
      
    }
    
    return *this;
    
  }
  
  ~proxy_updater(){
    if (NULL == mjava_updater) return;
    // delete reference to allow garbage collection
    JNIEnv *env = graphlab::jni_core<proxy_graph, proxy_updater>::get_jni_env();
    env->DeleteGlobalRef(mjava_updater);
    mjava_updater = NULL;
  }
  
  void operator()(icontext_type& context){

    JNIEnv *env = graphlab::jni_core<proxy_graph, proxy_updater>::get_jni_env();
    
    // retrieve application vertex ID
    jint app_vertex_id = context.vertex_data().app_id;
    
    // forward call to org.graphlab.Updater
    env->CallVoidMethod (mjava_updater, java_method_id,
                         &context,
                         app_vertex_id);
    
    // check for exception
    jthrowable exc = env->ExceptionOccurred();
    if (exc) {
    
      // TODO: better error handling
    
      logstream(LOG_ERROR)
        << "Exception occured!!"
        << std::endl;
        
      jclass new_exc;
      env->ExceptionDescribe();
      env->ExceptionClear();
      new_exc = env->FindClass("java/lang/IllegalArgumentException");
      if (new_exc == NULL) return;
      env->ThrowNew(new_exc, "thrown from C code");
      
    }

  }
  
};

/** A jni_core type that uses the proxy graph and the proxy updater */
typedef graphlab::jni_core<proxy_graph, proxy_updater> jni_core_type;

/** A context type that uses the proxy graph and the proxy updater */
typedef graphlab::iupdate_functor<proxy_graph, proxy_updater>::icontext_type icontext_type;

#endif