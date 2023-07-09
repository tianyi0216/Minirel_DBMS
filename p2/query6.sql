WITH S AS(
    SELECT DISTINCT(s.id)
    FROM Users s, Auctions a
    WHERE s.id = a.id
), B AS(
    SELECT DISTINCT(s.id)
    FROM Users s, Bids b
    WHERE s.id = b.id
)
SELECT COUNT(*)
FROM S, B
WHERE S.id = B.id;