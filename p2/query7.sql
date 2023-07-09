WITH Cat AS (
    SELECT DISTINCT(C.category)
    FROM Bids B, Categories C, Items I
    WHERE C.iid = I.iid AND I.iid = B.iid AND B.amount > 100
)
SELECT COUNT(*)
FROM Cat;