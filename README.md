# Roommate Expense Splitter

A C++17 terminal app for roommates who share rent, utility bills, groceries, and other household costs. Add an expense, choose who paid, and the app splits it equally among the current roommates and shows who owes whom.

## What you can do

- Add, remove, and view roommates. Someone who moves out keeps their existing balance and is left out of future expenses.
- Split rent, electricity, water, Wi-Fi, cooking gas, groceries, cleaning supplies, and repairs.
- See who paid each expense and what everyone owes.
- Record full or partial payments. The app shows **Everyone is settled up** when all balances reach zero.

## Run it

Open the repository folder in VS Code and select **Terminal → New Terminal**. In Windows PowerShell with MinGW g++ installed, run:

```powershell
g++ -std=c++17 -static main.cpp -o roommate-splitter.exe
if ($LASTEXITCODE -eq 0) { .\roommate-splitter.exe }
```

You can also build with CMake using the included `CMakeLists.txt`.

## Try an example

The app starts with Aarav, Ananya, Kabir, and six sample expenses. Choose **5** to see the expenses and **6** to see the balances. Ananya initially owes Aarav **Rs. 3,933.00**. If she pays him **Rs. 500.00**, record the payment with **7** and her remaining balance drops to **Rs. 3,433.00**.

Amounts are stored in integer paise. If a bill splits unevenly, the first roommates in the list get the extra paise so the shares add up exactly. Changes save automatically to a local `household.txt` file.
