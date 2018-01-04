/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2017-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/base/backtrace.h"
#include "hphp/runtime/base/tv-variant.h"
#include "hphp/runtime/ext/vsdebug/command.h"
#include "hphp/runtime/ext/vsdebug/debugger.h"

namespace HPHP {
namespace VSDEBUG {

ScopesCommand::ScopesCommand(
  Debugger* debugger,
  folly::dynamic message
) : VSCommand(debugger, message),
    m_frameId{0} {

  const folly::dynamic& args = tryGetObject(message, "arguments", s_emptyArgs);
  const int frameId = tryGetInt(args, "frameId", -1);
  if (frameId <= 0) {
    throw DebuggerCommandException("Invalid frameId specified.");
  }

  m_frameId = frameId;
}

ScopesCommand::~ScopesCommand() {
}

FrameObject* ScopesCommand::getFrameObject(DebuggerSession* session) {
  if (m_frameObj != nullptr) {
    return m_frameObj;
  }

  m_frameObj = session->getFrameObject(m_frameId);
  return m_frameObj;
}

int64_t ScopesCommand::targetThreadId(DebuggerSession* session) {
  FrameObject* frame = getFrameObject(session);
  if (frame == nullptr) {
    return 0;
  }

  return frame->m_requestId;
}

bool ScopesCommand::executeImpl(
  DebuggerSession* session,
  folly::dynamic* responseMsg
) {

  folly::dynamic body = folly::dynamic::object;
  folly::dynamic scopes = folly::dynamic::array;

  if (getFrameObject(session) != nullptr) {
    scopes.push_back(getScopeDescription(session, "Locals", ScopeType::Locals));
    scopes.push_back(getScopeDescription(session,
                                         "Superglobals",
                                         ScopeType::Superglobals));
    scopes.push_back(getScopeDescription(session,
                                         "User Defined Constants",
                                         ScopeType::UserDefinedConstants));
    scopes.push_back(getScopeDescription(session,
                                         "System Constants",
                                         ScopeType::CoreConstants));
  }

  body["scopes"] = scopes;
  (*responseMsg)["body"] = body;

  // Completion of this command does not resume the target.
  return false;
}

folly::dynamic ScopesCommand::getScopeDescription(
  DebuggerSession* session,
  const char* displayName,
  ScopeType type
) {
  FrameObject* frame = getFrameObject(session);
  assert (frame != nullptr);

  int req = frame->m_requestId;
  int depth = frame->m_frameDepth;

  folly::dynamic scope = folly::dynamic::object;
  unsigned int scopeId = session->generateScopeId(req, depth, type);

  scope["name"] = displayName;
  scope["variablesReference"] = scopeId;
  scope["expensive"] = true;

  const ScopeObject* scopeObj = session->getScopeObject(scopeId);
  scope["namedVariables"] = VariablesCommand::countScopeVariables(scopeObj);

  return scope;
}

}
}
