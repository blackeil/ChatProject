# AGENTS.md

## Repo scope
This repo is a C++ backend project with authentication and chat-related modules.

Important directories and files:
- include/
- src/
- README.md

For username bloom filter tasks, focus on:
- include/mysql_user_repo.h
- src/mysql_user_repo.cpp
- include/handlers_auth.h
- src/handlers_auth.cpp
- main / server bootstrap file
- new files:
  - include/username_bloom_filter.h
  - src/username_bloom_filter.cpp

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
- For the Bloom filter, the basic framework has already been developed.
- Use std::vector<uint64_t> for the bit array unless the task explicitly requests otherwise.
- Keep concurrency design practical: read-heavy, write-light usage should be considered.
- The Bloom filter was updated after the registration was successful. Please check it
- Service startup should support rebuilding the bloom filter from all usernames in MySQL.

## Safety and correctness rules
- Never log plaintext passwords.
- Do not accept auth token from URL query parameters.
- Prefer prepared statements over manual SQL string concatenation when touching SQL code.
- Do not let bloom filter decide registration success directly.
- If bloom filter says "definitely not present", login may fail fast.
- If bloom filter says "possibly present", MySQL must still be queried.

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