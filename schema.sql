CREATE TABLE IF NOT EXISTS roommates (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    room VARCHAR(30) NOT NULL,
    active BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS expenses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    category VARCHAR(40) NOT NULL,
    note VARCHAR(150) NOT NULL DEFAULT '',
    amount BIGINT NOT NULL CHECK (amount > 0),
    paid_by INT NOT NULL,
    expense_date DATE NOT NULL,
    FOREIGN KEY (paid_by) REFERENCES roommates(id)
);

CREATE TABLE IF NOT EXISTS expense_shares (
    expense_id INT NOT NULL,
    roommate_id INT NOT NULL,
    amount BIGINT NOT NULL CHECK (amount >= 0),
    PRIMARY KEY (expense_id, roommate_id),
    FOREIGN KEY (expense_id) REFERENCES expenses(id),
    FOREIGN KEY (roommate_id) REFERENCES roommates(id)
);

CREATE TABLE IF NOT EXISTS payments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender INT NOT NULL,
    receiver INT NOT NULL,
    amount BIGINT NOT NULL CHECK (amount > 0),
    payment_date DATE NOT NULL,
    CHECK (sender <> receiver),
    FOREIGN KEY (sender) REFERENCES roommates(id),
    FOREIGN KEY (receiver) REFERENCES roommates(id)
);

CREATE TABLE IF NOT EXISTS household (
    id INT PRIMARY KEY,
    seeded BOOLEAN NOT NULL DEFAULT FALSE
);
