# AGENTS.md

## Repo scope
This repo is a C++ backend project with authentication and chat-related modules.

Important directories and files:
- include/
- src/
- README.md

## Current project facts
- HTTP currently uses httplib; do not replace the HTTP layer unless the task explicitly asks for it.
- MySQL stores user account data.
- Redis stores session data.
- Username matching is case-sensitive.
- Bloom filter is only a pre-check before MySQL lookup, not a source of truth.
- Final existence check must still rely on MySQL unique constraint / query result.

## Coding rules
- Prefer minimal diffs.
- Do not refactor unrelated modules.
- Do not modify build output or third-party directories.
- Keep naming simple and consistent with existing code style.
- Do not introduce overly abstract class hierarchies for small features.
- Keep concurrency design practical: read-heavy, write-light usage should be considered.

## Safety and correctness rules
- Never log plaintext passwords.
- Do not accept auth token from URL query parameters.
- Prefer prepared statements over manual SQL string concatenation when touching SQL code.
- bloom_filer has already been developed.

## Task workflow
For non-trivial tasks:
1. Read only the files relevant to the task.
2. Summarize the current implementation briefly.
3. Propose the minimal implementation plan.
4. Then write code.

For code changes:
- Show the exact files to modify.
- Keep the change scoped.
- Explain any assumptions clearly.

## Done when
A task is complete only if:
- The changed code is limited to the requested scope.
- Existing register/login/session behavior is preserved unless explicitly changed.
- New interfaces are complete and usable.
- The implementation matches the requested architecture.
- Output includes a short summary of what changed.

## HTTP migration rules

Current goal:
Replace cpp-httplib with a self-implemented HTTP/1.1 server based on epoll + Reactor + non-blocking sockets.

Scope for phase 1:
- Keep existing auth business behavior unchanged.
- Reuse service/repo layers whenever possible.
- Build only the minimum HTTP/1.1 feature set needed by current auth APIs.

Do not implement yet:
- WebSocket
- TLS/SSL
- HTTP/2
- chunked request body
- multipart/form-data
- chat messaging

Architecture constraints:
- Separate net/, http/, auth/, user/, repo/, config/ modules (has already been developed.)
- I/O concerns must stay in net/http layers
- business logic must stay in service/repo layers
- prefer minimal diffs and incremental milestones
- first make single-reactor work, then add worker thread pool later