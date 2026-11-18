# Roommate Splitter

A full-stack dashboard for roommates who share rent, utility bills, groceries, and other household costs. The frontend uses HTML, CSS, and JavaScript. The Python server uses MySQL Connector/Python to save the household in MySQL.

## What you can do

- View monthly spending, outstanding balances, and spending by category.
- Add and remove roommates. Removing someone excludes them from new expenses while keeping their old shares and balances.
- Add expenses and see exactly how each one was shared. New expenses split equally among current roommates, with any leftover paise assigned in roster order.
- Record full or partial payments and see payment history. When all outstanding balances reach zero, the dashboard shows **Everyone is settled up**.

On first launch, a new database is filled with Aarav, Ananya, Kabir, and six sample expenses. The example starts with Ananya owing Aarav ₹3,933.00 and Kabir owing Aarav ₹6,444.00.

## Run it in VS Code

You need Python 3.10 or newer and a running MySQL server. Open the project folder in VS Code, then open a PowerShell terminal.

### 1. Create the database and an app user

Connect to your local MySQL server as an administrator using MySQL Workbench or the MySQL command line. Run this SQL once. Replace the example password with your own password.

```sql
CREATE DATABASE IF NOT EXISTS roommate_splitter CHARACTER SET utf8mb4;
CREATE USER IF NOT EXISTS 'roommate_app'@'localhost' IDENTIFIED BY 'choose-your-local-password';
GRANT SELECT, INSERT, UPDATE, CREATE, REFERENCES
ON roommate_splitter.* TO 'roommate_app'@'localhost';
```

If that MySQL user already exists, use its current password. The app creates its own tables on startup.

### 2. Install the connector and start the server

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
$env:MYSQL_PASSWORD = Read-Host "MySQL app user password" -MaskInput
.\.venv\Scripts\python.exe app.py
```

Open [http://127.0.0.1:8000](http://127.0.0.1:8000). Use **Ctrl+C** in the terminal to stop the server. If port 8000 is busy, pass `--port 8001` to `app.py`.

Set `MYSQL_HOST`, `MYSQL_PORT`, `MYSQL_USER`, or `MYSQL_DATABASE` in the same terminal if your setup differs. The defaults are `127.0.0.1`, `3306`, `roommate_app`, and `roommate_splitter`.

## How the project is organized

| File | Purpose |
| --- | --- |
| `app.py` | Serves the frontend and validates the API requests. |
| `database.py` | Connects to MySQL, records changes in transactions, and calculates balances. |
| `schema.sql` | Defines the roommates, expenses, shares, and payments tables. |
| `static/` | Contains the dashboard and its forms, styling, and API calls. |

Money is stored as integer paise. Expense shares record who participated when the expense was added. Payments reduce what a roommate owes without changing anyone's original expense share. This version is a local app for one household. It starts with a new MySQL database and uses the built-in demo records.

The original standalone C++ demo remains in main.cpp and CMakeLists.txt. The web dashboard uses the Python backend and MySQL.
