/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/saschazelzer/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/


#include "usTrackedService_p.h"

#include "usBundleContext.h"
#include "usLDAPFilter.h"

#include <stdexcept>

namespace us {

template<class S, class TTT>
ServiceTrackerPrivate<S,TTT>::ServiceTrackerPrivate(
    ServiceTracker<S,T>* st, BundleContext* context,
    const ServiceReference<S>& reference,
    ServiceTrackerCustomizer<S,T>* customizer)
  : context(context), customizer(customizer), trackReference(reference),
    trackedService(), cachedReference(), cachedService(), q_ptr(st)
{
  this->customizer = customizer ? customizer : q_func();
  std::stringstream ss;
  ss << "(" << ServiceConstants::SERVICE_ID() << "="
     << any_cast<long>(reference.GetProperty(ServiceConstants::SERVICE_ID())) << ")";
  this->listenerFilter = ss.str();
  try
  {
    this->filter = LDAPFilter(listenerFilter);
  }
  catch (const std::invalid_argument& e)
  {
    /*
     * we could only get this exception if the ServiceReference was
     * invalid
     */
    std::invalid_argument ia(std::string("unexpected std::invalid_argument exception: ") + e.what());
    throw ia;
  }
}

template<class S, class TTT>
ServiceTrackerPrivate<S,TTT>::ServiceTrackerPrivate(
    ServiceTracker<S,T>* st,
    BundleContext* context, const std::string& clazz,
    ServiceTrackerCustomizer<S,T>* customizer)
      : context(context), customizer(customizer), trackClass(clazz),
        trackReference(), trackedService(), cachedReference(),
        cachedService(), q_ptr(st)
{
  this->customizer = customizer ? customizer : q_func();
  this->listenerFilter = std::string("(") + us::ServiceConstants::OBJECTCLASS() + "="
                        + clazz + ")";
  try
  {
    this->filter = LDAPFilter(listenerFilter);
  }
  catch (const std::invalid_argument& e)
  {
    /*
     * we could only get this exception if the clazz argument was
     * malformed
     */
    std::invalid_argument ia(
        std::string("unexpected std::invalid_argument exception: ") + e.what());
    throw ia;
  }
}

template<class S, class TTT>
ServiceTrackerPrivate<S,TTT>::ServiceTrackerPrivate(
    ServiceTracker<S,T>* st,
    BundleContext* context, const LDAPFilter& filter,
    ServiceTrackerCustomizer<S,T>* customizer)
      : context(context), filter(filter), customizer(customizer),
        listenerFilter(filter.ToString()), trackReference(),
        trackedService(), cachedReference(), cachedService(), q_ptr(st)
{
  this->customizer = customizer ? customizer : q_func();
  if (context == nullptr)
  {
    throw std::invalid_argument("The bundle context cannot be null.");
  }
}

template<class S, class TTT>
ServiceTrackerPrivate<S,TTT>::~ServiceTrackerPrivate()
{

}

template<class S, class TTT>
std::vector<ServiceReference<S> > ServiceTrackerPrivate<S,TTT>::GetInitialReferences(
  const std::string& className, const std::string& filterString)
{
  std::vector<ServiceReference<S> > result;
  std::vector<ServiceReferenceU> refs = context->GetServiceReferences(className, filterString);
  for(std::vector<ServiceReferenceU>::const_iterator iter = refs.begin();
      iter != refs.end(); ++iter)
  {
    ServiceReference<S> ref(*iter);
    if (ref)
    {
      result.push_back(ref);
    }
  }
  return result;
}

template<class S, class TTT>
void ServiceTrackerPrivate<S,TTT>::GetServiceReferences_unlocked(std::vector<ServiceReference<S> >& refs, TrackedService<S,TTT>* t) const
{
  if (t->Size_unlocked() == 0)
  {
    return;
  }
  t->GetTracked_unlocked(refs);
}

template<class S, class TTT>
TrackedService<S,TTT>* ServiceTrackerPrivate<S,TTT>::Tracked() const
{
  return trackedService.get();
}

template<class S, class TTT>
void ServiceTrackerPrivate<S,TTT>::Modified()
{
  cachedReference.Store(ServiceReference<S>()); /* clear cached value */
  cachedService.Store(std::shared_ptr<TrackedParmType>()); /* clear cached value */
  DIAG_LOG(*context->GetLogSink()) << "ServiceTracker::Modified(): " << filter;
}

}
