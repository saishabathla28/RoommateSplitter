# Roommate Expense Splitter

A small C++17 console project. It opens with Aarav, Ananya, and Kabir and six shared expenses.

## Run from VS Code

Open this folder and select **Terminal → New Terminal**. In PowerShell, run:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -static main.cpp -o roommate-splitter.exe
if ($LASTEXITCODE -eq 0) { .\roommate-splitter.exe }
```

If PowerShell cannot find g++, use `C:\msys64\ucrt64\bin\g++.exe` in the first command. The included CMake file also builds the same program.

Choose **3** to see the roommates, **5** to see the sample expenses, or **6** to see who owes whom. After someone transfers money, choose **7** to record the payment. Partial payments leave the remaining balance visible.

New expenses are split equally among current roommates. Leftover paise go to the first roommates on the list. Removing someone hides them from the roster and excludes them from new expenses. Their earlier shares and balances remain available.

Changes save automatically to `household.txt` in the current folder. Run `.\roommate-splitter.exe practice.txt` to try a separate household.
