// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/browser.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "base/command_line.h"
#include "net/proxy/proxy_resolver_v8.h"

#if defined(OS_WIN)
#include "ui/gfx/win/dpi.h"
#endif

#if defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_util.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = NULL;

AtomBrowserMainParts::AtomBrowserMainParts()
    : atom_bindings_(new AtomBindings),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(true)),
      isolate_(v8::Isolate::GetCurrent()),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_(isolate_, v8::Context::New(isolate_)),
      context_scope_(v8::Local<v8::Context>::New(isolate_, context_)) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

brightray::BrowserContext* AtomBrowserMainParts::CreateBrowserContext() {
  return new AtomBrowserContext();
}

void AtomBrowserMainParts::InitProxyResolverV8() {
  // Since we are integrating node in browser, we can just be sure that an
  // V8 instance would be prepared, while the ProxyResolverV8::CreateIsolate()
  // would try to create a V8 isolate, which messed everything on Windows, so
  // we have to override and call RememberDefaultIsolate on Windows instead.
  net::ProxyResolverV8::RememberDefaultIsolate();
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

  node_bindings_->Initialize();

  // Create the global environment.
  global_env = node_bindings_->CreateEnvironment(
      v8::Local<v8::Context>::New(isolate_, context_));

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(isolate_, global_env->process_object());
}

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  brightray::BrowserMainParts::PreMainMessageLoopRun();

#if defined(USE_X11)
  libgtk2ui::GtkInitFromCommandLine(*CommandLine::ForCurrentProcess());
#endif

  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

  // Make sure the url request job factory is created before the
  // will-finish-launching event.
  static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
      GetRequestContext();

#if !defined(OS_MACOSX)
  // The corresponding call in OS X is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching();
#endif
}

}  // namespace atom
