<table>
  <tr>
    <td align="center">
        CI status
    </td>
    <td align="center">
        Debian package
    </td>
  </tr>
  <tr>
    <td align="center">
        <a href="https://github.com/asherikov/thread_supervisor/actions/workflows/master.yml">
        <img src="https://github.com/asherikov/thread_supervisor/actions/workflows/master.yml/badge.svg" alt="Build Status">
        </a>
    </td>
    <td align="center">
        <a href="https://cloudsmith.io/~asherikov-aV7/repos/all/packages/detail/deb/thread-supervisor/latest/a=all;d=any-distro%252Fany-version;t=binary/">
        <img src="https://api-prd.cloudsmith.io/v1/badges/version/asherikov-aV7/all/deb/thread-supervisor/latest/a=all;d=any-distro%252Fany-version;t=binary/?render=true&show_latest=true" alt="Latest version of 'thread-supervisor' @ Cloudsmith" />
        </a>
    </td>
  </tr>
</table>


Simple C++17 thread supervisor which automatically restarts failed or finished threads.

Doxygen documentation: https://asherikov.github.io/thread_supervisor/doxygen

Dependencies:
- C++17 compatible compiler
- POSIX threads (pthreads)
- Boost.UTF for tests

Integrate in a `cmake` project with:
```
find_package(thread_supervisor REQUIRED)
target_link_libraries(... thread_supervisor::thread_supervisor)
```

Example: https://asherikov.github.io/thread_supervisor/doxygen/DEMO.html [`./test/supervisor.cpp`]

