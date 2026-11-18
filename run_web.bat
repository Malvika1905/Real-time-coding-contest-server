@echo off
title Real-Time Coding Contest Server (C++ & React)
echo ========================================================================
echo        REAL-TIME CODING CONTEST SERVER: C++ BACKEND & REACT WEB UI      
echo ========================================================================

echo.
echo Starting React Web Application on http://localhost:5173 ...
echo.

cd frontend
call npm.cmd run dev -- --host
pause
