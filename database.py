import os
from contextlib import contextmanager
from datetime import date
from pathlib import Path

import mysql.connector

CATEGORIES = (
    "Rent", "Electricity", "Water", "Wi-Fi", "Cooking gas", "Groceries",
    "Cleaning supplies", "Repairs and maintenance", "Other",
)


@contextmanager
def transaction(write=False):
    connection = mysql.connector.connect(
        host=os.getenv("MYSQL_HOST", "127.0.0.1"),
        port=int(os.getenv("MYSQL_PORT", "3306")),
        user=os.getenv("MYSQL_USER", "roommate_app"),
        password=os.getenv("MYSQL_PASSWORD", ""),
        database=os.getenv("MYSQL_DATABASE", "roommate_splitter"),
        connection_timeout=5,
    )
    cursor = connection.cursor(dictionary=True)
    try:
        if write:
            # Serialize changes to the roster and ledger for this household.
            cursor.execute("SELECT id FROM household WHERE id = 1 FOR UPDATE")
            cursor.fetchall()
        yield cursor
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        cursor.close()
        connection.close()


def insert_expense(cursor, category, note, amount, payer, day, participants):
    cursor.execute(
        """INSERT INTO expenses (category, note, amount, paid_by, expense_date)
           VALUES (%s, %s, %s, %s, %s)""",
        (category, note, amount, payer, day),
    )
    expense = cursor.lastrowid
    share, remainder = divmod(amount, len(participants))
    cursor.executemany(
        "INSERT INTO expense_shares VALUES (%s, %s, %s)",
        [(expense, person, share + int(i < remainder))
         for i, person in enumerate(participants)],
    )


def initialize():
    schema = Path(__file__).with_name("schema.sql").read_text(encoding="utf-8")
    with transaction() as cursor:
        for statement in schema.split(";"):
            if statement.strip():
                cursor.execute(statement)
        cursor.execute("INSERT IGNORE INTO household (id) VALUES (1)")
    with transaction(write=True) as cursor:
        cursor.execute("SELECT seeded FROM household WHERE id = 1")
        if cursor.fetchone()["seeded"]:
            return
        roommates = []
        for name, room in (("Aarav Sharma", "1"), ("Ananya Rao", "2"), ("Kabir Singh", "3")):
            cursor.execute("INSERT INTO roommates (name, room) VALUES (%s, %s)", (name, room))
            roommates.append(cursor.lastrowid)
        samples = [
            ("Rent", "Apartment rent", 1_800_000, 0),
            ("Electricity", "Monthly electricity bill", 237_000, 1),
            ("Water", "Building water charge", 54_000, 2),
            ("Wi-Fi", "Home internet plan", 99_900, 2),
            ("Groceries", "Shared pantry restock", 168_000, 1),
            ("Cleaning supplies", "Detergent and floor cleaner", 36_000, 0),
        ]
        for category, note, amount, payer in samples:
            insert_expense(cursor, category, note, amount, roommates[payer], date.today(), roommates)
        cursor.execute("UPDATE household SET seeded = TRUE WHERE id = 1")


def fetch(cursor, query, parameters=()):
    cursor.execute(query, parameters)
    return cursor.fetchall()


def balances(cursor):
    rows = fetch(cursor, """
        SELECT person, SUM(delta) AS balance FROM (
            SELECT paid_by AS person, amount AS delta FROM expenses
            UNION ALL SELECT roommate_id, -amount FROM expense_shares
            UNION ALL SELECT sender, amount FROM payments
            UNION ALL SELECT receiver, -amount FROM payments
        ) AS entries GROUP BY person
    """)
    return {row["person"]: int(row["balance"]) for row in rows}


def settlements(net):
    debtors = [[person, -value] for person, value in sorted(net.items()) if value < 0]
    creditors = [[person, value] for person, value in sorted(net.items()) if value > 0]
    result = []
    i = j = 0
    while i < len(debtors) and j < len(creditors):
        amount = min(debtors[i][1], creditors[j][1])
        result.append({"sender": debtors[i][0], "receiver": creditors[j][0], "amount": amount})
        debtors[i][1] -= amount
        creditors[j][1] -= amount
        i += debtors[i][1] == 0
        j += creditors[j][1] == 0
    return result


def dashboard():
    with transaction() as cursor:
        people = fetch(cursor, "SELECT * FROM roommates ORDER BY id")
        expenses = fetch(cursor, "SELECT * FROM expenses ORDER BY expense_date DESC, id DESC")
        payments = fetch(cursor, "SELECT * FROM payments ORDER BY payment_date DESC, id DESC")
        shares = {}
        for row in fetch(cursor, "SELECT * FROM expense_shares ORDER BY roommate_id"):
            shares.setdefault(row["expense_id"], []).append(row)
        for expense in expenses:
            expense["shares"] = shares.get(expense["id"], [])
        net = balances(cursor)
        for person in people:
            person["balance"] = net.get(person["id"], 0)
        return {"roommates": people, "expenses": expenses, "payments": payments,
                "settlements": settlements(net), "categories": CATEGORIES}


def add_roommate(name, room):
    with transaction(write=True) as cursor:
        cursor.execute("INSERT INTO roommates (name, room) VALUES (%s, %s)", (name, room))


def remove_roommate(person):
    with transaction(write=True) as cursor:
        cursor.execute("UPDATE roommates SET active = FALSE WHERE id = %s AND active = TRUE", (person,))
        if cursor.rowcount == 0:
            raise ValueError("This roommate is already inactive or could not be found.")


def add_expense(category, note, amount, payer, day):
    with transaction(write=True) as cursor:
        active = fetch(cursor, "SELECT id FROM roommates WHERE active = TRUE ORDER BY id")
        participants = [row["id"] for row in active]
        if payer not in participants:
            raise ValueError("Choose a current roommate as the payer.")
        insert_expense(cursor, category, note, amount, payer, day, participants)


def add_payment(sender, receiver, amount, day):
    with transaction(write=True) as cursor:
        net = balances(cursor)
        if sender == receiver or amount > -net.get(sender, 0) or amount > net.get(receiver, 0):
            raise ValueError("The payment must fit both outstanding balances.")
        cursor.execute(
            "INSERT INTO payments (sender, receiver, amount, payment_date) VALUES (%s, %s, %s, %s)",
            (sender, receiver, amount, day),
        )
