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
        TODO
    </td>
  </tr>
</table>


Simple C++11 thread supervisor which automatically restarts failed or finished threads.

Doxygen documentation: https://asherikov.github.io/thread_supervisor/doxygen

Dependencies:
- C++11 compatible compiler
- POSIX threads (pthreads)
- Boost.UTF for tests

Integrate in a `cmake` project with:
```
find_package(thread_supervisor REQUIRED)
target_link_libraries(... thread_supervisor::thread_supervisor)
```

Example: https://asherikov.github.io/thread_supervisor/doxygen/DEMO.html [`./test/supervisor.cpp`]

