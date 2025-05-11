@echo off
echo Создание виртуального окружения Python...
python -m venv .venv

echo Активация виртуального окружения...
call .venv\Scripts\activate.bat

echo Установка зависимостей...
pip install -r requirements.txt

echo.
echo Виртуальное окружение настроено!
echo Для активации используйте команду: .venv\Scripts\activate.bat
echo. 