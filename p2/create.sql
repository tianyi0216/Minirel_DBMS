DROP TABLE IF EXISTS Items;
DROP TABLE IF EXISTS Auctions;
DROP TABLE IF EXISTS Categories;
DROP TABLE IF EXISTS Users;
DROP TABLE IF EXISTS Bids;

CREATE TABLE Items(
    iid INTEGER PRIMARY KEY,
    Name CHAR(255) NOT NULL,
    Buy_Price DECIMAL(20,20),
    First_Bid DECIMAL(20, 20) NOT NULL,
    Description TEXT NOT NULL
);

CREATE TABLE Categories(
    iid INTEGER NOT NULL,
    Category CHAR(50) NOT NULL,
    PRIMARY KEY(iid, Category)
    FOREIGN KEY (iid) REFERENCES Items(iid)
);

CREATE TABLE Users(
    id CHAR(255) PRIMARY KEY,
    Rating INTEGER NOT NULL,
    Location CHAR(255),
    Country CHAR(70)
);

CREATE TABLE Bids(
    id CHAR(255) NOT NULL,
    iid INTEGER NOT NULL,
    Bidtime DATETIME NOT NULL,
    Amount DECIMAL(20,20) NOT NULL,
    PRIMARY KEY(id, iid, Bidtime),
    FOREIGN KEY (id) REFERENCES User(id),
    FOREIGN KEY (iid) REFERENCES Items(iid)
);

CREATE TABLE Auctions(
    iid INTEGER NOT NULL,
    id CHAR(255) NOT NULL,
    Current_Price DECIMAL(20,20) NOT NULL,
    Num_Bids INTEGER NOT NULL,
    StartDate DATETIME NOT NULL,
    EndDate DATETIME NOT NULL,
    PRIMARY KEY(id, iid),
    FOREIGN KEY (id) REFERENCES User(id),
    FOREIGN KEY (iid) REFERENCES Items(iid)
);