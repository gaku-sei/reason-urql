# Hooks

In this section, we cover the main mechanism for requesting data in `reason-urql` – hooks!

`reason-urql` comes with a set of custom hooks to use in your ReasonReact components, including `useQuery`, `useMutation`, `useDynamicMutation`, and `useSubscription`. These are fully type safe and will automatically infer the type of your GraphQL response if using `graphql_ppx_re` or `graphql_ppx`.

## `useQuery`

`useQuery` allows you to execute a GraphQL query.

### Arguments

| Argument        | Type                                    | Description                                                                                                                                                                                                                                                             |
| --------------- | --------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `request`       | `UrqlTypes.request`                     | The `graphql_ppx` request representing your query. Generated by calling `.make()` on the `graphql_ppx` module. If you're not using `graphql_ppx`, pass a `Js.t` of the following shape: `{. "query": string, "variables": Js.Json.t, "parse": Js.Json.t => 'response }` |
| `requestPolicy` | `UrqlTypes.requestPolicy=?`             | Optional. The request policy to use to execute the query. Defaults to `"cache-first"`.                                                                                                                                                                                  |
| `pause`         | `bool=?`                                | Optional. A boolean flag instructing `useQuery` to pause execution of the subsequent query operation.                                                                                                                                                                   |
| `pollInterval`  | `int=?`                                 | Optional. Instructs the hook to reexecute the query every `pollInterval` milliseconds.                                                                                                                                                                                  |
| `context`       | `ClientTypes.partialOperationContext=?` | Optional. A partial operation context object for modifying the execution parameters of this particular query (i.e. using different `fetchOptions` or a different `url`).                                                                                                |

### Return Type

`useQuery` returns a tuple containing the result of executing your GraphQL query, `result`, and a function for re-executing the query imperatively, `executeQuery`.

| Return Value   | Type                                                              | Description                                                                                                                                                                                                                       |
| -------------- | ----------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `result`       | `UrqlTypes.hookResponse('response)`                               | A record containing fields for `fetching`, `data`, `error`, and `response`. `response` is a variant containing constructors for `Data`, `Error`, `Fetching` and `NotFound`. Useful for pattern matching to render conditional UI. |
| `executeQuery` | `(~context: ClientTypes.partialOperationContext=?, unit) => unit` | A function for imperatively re-executing the query. Accepts a single optional labeled argument, `context`, for altering the query execution. Use `ClientTypes.createOperationContext` to supply the `context` argument.           |

### Example

```reason
open ReasonUrql;

module GetPokémon = [%graphql
  {|
  query pokémon($name: String!) {
    pokemon(name: $name) {
      name
      classification
      image
    }
  }
|}
];

[@react.component]
let make = () => {
  let request = GetPokémon.make(~name="Butterfree", ());
  let (Hooks.{response}, executeQuery) = Hooks.useQuery(~request, ());

  switch (response) {
    | Fetching => <div> "Loading"->React.string </div>
    | Data(d) =>
      <div>
         <img src=d##pokemon##image>
         <span> d##pokemon##name->React.string </span>
         <span> d##pokemon##classification->React.string </span>
         <button
          onClick={_e =>
            executeQuery(
              ~context=ClientTypes.createOperationContext(
                ~requestPolicy=`CacheOnly,
                ()
              ),
              ()
            )}
         >
          {j|Refetch data for $d##pokemon##name|j} -> React.string
         </button>
      </div>
    | Error(_e) => <div> "Error"->React.string </div>
    | NotFound => <div> "NotFound"->React.string </div>
  }
}
```

Check out `examples/2-query` to see an example of using the `useQuery` hook.

## `useMutation`

`useMutation` allows you to execute a GraphQL mutation via the returned `executeMutation` function.

### Arguments

| Argument  | Type                | Description                                                                                                                                                                                                                                                                |
| --------- | ------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `request` | `UrqlTypes.request` | The `graphql_ppx` request representing your mutation. Generated by calling `.make()` on the `graphql_ppx` module. If you're not using `graphql_ppx`, pass a `Js.t` of the following shape: `{. "query": string, "variables": Js.Json.t, "parse": Js.Json.t => 'response }` |

### Return Type

`useMutation` returns a tuple containing the result of executing your GraphQL mutation as a record, `result`, and a function for executing the mutation imperatively, `executeMutation`. By default, `useMutation` **does not** execute your mutation when your component renders – rather, it is up to you to call `executeMutation` when you want to by attaching it to on an event handler or running it inside of an effect. Read more on the [`urql` docs](https://formidable.com/open-source/urql/docs/basics/mutations/#sending-a-mutation).

| Return Value      | Type                                                                                                   | Description                                                                                                                                                                                                                       |
| ----------------- | ------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `result`          | `UrqlTypes.hookResponse('response)`                                                                    | A record containing fields for `fetching`, `data`, `error`, and `response`. `response` is a variant containing constructors for `Data`, `Error`, `Fetching` and `NotFound`. Useful for pattern matching to render conditional UI. |
| `executeMutation` | `(~context: ClientTypes.partialOperationContext=?, unit) => Js.Promise.t(ClientTypes.operationResult)` | A function for imperatively executing the mutation. Accepts a single labeled argument, `context`, for altering the mutation execution. Use `ClientTypes.createOperationContext` to supply the `context` argument.                 |

### Example

```reason
open ReasonUrql;

module LikeDog = [%graphql
    {|
    mutation likeDog($key: ID!) {
      likeDog(key: $key) {
        likes
      }
    }
  |}
  ];

[@react.component]
let make = (~id) => {
  let request = LikeDog.make(~key=id, ());
  let (_, executeMutation) = Hooks.useMutation(~request);

  <button onClick={_e => executeMutation() |> ignore}>
    "Like This Dog!"->React.string
  </button>
}
```

Check out `examples/3-mutation` to see an example of using the `useMutation` hook.

## `useDynamicMutation`

`useDynamicMutation` is quite similar to `useMutation`, except it is a hook reserved for when you want to **dynamically** pass in variables to the `executeMutation` function _at execution time_. In constrast, `useMutation` applies variables immediately when the hook is called (_before executation time_).

A good example of a case where `useDynamicMutation` comes in handy is when you need to execute a mutation with variables retrieved from `useQuery` in the same component. See the Example section below for guidance on this technique.

### Arguments

| Argument     | Type                                                            | Description                                                                                           |
| ------------ | --------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `definition` | `(Js.Json.t => 'response, string, UrqlTypes.graphqlDefinition)` | The definition of your Graphql mutation. If using `graphql_ppx`, this is `MutationModule.definition`. |

### Return Type

`useDynamicMutation` returns nearly the same tuple as `useMutation`, containing the result of executing your GraphQL mutation as a record, `result`, and a function for executing the mutation imperatively, `executeMutation`. Unlike `useMutation`, the `executeMutation` function returned by `useDynamicMutation` accepts all your variables as named arguments.

| Return Value      | Type                                                                                                                           | Description                                                                                                                                                                                                                                                                                                                                                                                                 |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `result`          | `UrqlTypes.hookResponse('response)`                                                                                            | A record containing fields for `fetching`, `data`, `error`, and `response`. `response` is a variant containing constructors for `Data`, `Error`, `Fetching` and `NotFound`. Useful for pattern matching to render conditional UI.                                                                                                                                                                           |
| `executeMutation` | `(~myVar1, ~myVar2, ..., ~context: ClientTypes.partialOperationContext)=?, unit) => Js.Promise.t(ClientTypes.operationResult)` | A function for imperatively executing the mutation, which accepts all the variables of your mutation as named arguments. Also accepts an optional `partialOperationContext` using the labeled argument `context`. Use `ClientTypes.createOperationContext` to supply the `context` argument. You must use a final positional `unit` argument, `()`, to indicate that you've finished applying the function. |

### Example

```reason
open ReasonUrql;

module GetAllDogs = [%graphql {|
  query dogs {
    dogs {
      name
      breed
      likes
    }
  }
|}];

module LikeDog = [%graphql
    {|
    mutation likeDog($key: ID!) {
      likeDog(key: $key) {
        likes
      }
    }
  |}
  ];

[@react.component]
let make = () => {
  let (Hooks.{ response }) = Hooks.useQuery(~request=GetAllDogs.make(), ());
  let (_, executeMutation) = Hooks.useDynamicMutation(LikeDog.definition);

  switch (response) {
    | Data(d) => {
      <button
      onClick={
        _e => executeMutation(
          ~key=d##dogs[0]##id,
          ~context=ClientTypes.createOperationContext(
            ~requestPolicy=`NetworkOnly,
            ()
          ),
          ()
          ) |> ignore}
      >
        "Like The First Dog!"->React.string
      </button>
    }
    | _ => React.null
  }
}
```

Check out `examples/3-mutation` to see an example of using the `useDynamicMutation` hook.

## `useSubscription`

`useSubscription` allows you to execute a GraphQL subscription. You can accumulate the results of executing subscriptions by passing a `handler` function to `useSubscription`.

If using the `useSubscription` hook, be sure your client is configured with the [`subscriptionExchange`](./exchanges#subscription-exchange).

### Arguments

| Argument  | Type                | Description                                                                                                                                                                                                                                                                    |
| --------- | ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `request` | `UrqlTypes.request` | The `graphql_ppx` request representing your subscription. Generated by calling `.make()` on the `graphql_ppx` module. If you're not using `graphql_ppx`, pass a `Js.t` of the following shape: `{. "query": string, "variables": Js.Json.t, "parse": Js.Json.t => 'response }` |
| `handler` | `Hooks.handler`     | Optional. A variant type to allow for proper type inference of accumulated subscription data. A `handler` function allows you to accumulate subscription responses in the `data` field of the returned state record.                                                           |

### Return Type

`useSubscription` returns a tuple containing the result of executing your GraphQL subscription, `result`, and a function for re-executing the subscription imperatively, `executeSubscription`.

| Return Value          | Type                                                         | Description                                                                                                                                                                                                                           |
| --------------------- | ------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `result`              | `UrqlTypes.hookResponse('response)`                          | A record containing fields for `fetching`, `data`, `error`, and `response`. `response` is a variant containing constructors for `Data`, `Error`, `Fetching` and `NotFound`. Useful for pattern matching to render conditional UI.     |
| `executeSubscription` | `(~context: Client.partialOperationContext=?, unit) => unit` | A function for imperatively re-executing the subscription. Accepts a single optional labeled argument, `context`, for altering the subscription execution. Use `ClientTypes.createOperationContext` to supply the `context` argument. |

### Example

```reason
open ReasonUrql;

module SubscribeRandomInt = [%graphql
  {|
  subscription subscribeNumbers {
    newNumber @bsDecoder(fn: "string_of_int")
  }
|}
];

/* Accumulate subscriptions as new values arrive from your GraphQL endpoint. */
let handler = (prevSubscriptions, subscription) => {
  switch (prevSubscriptions) {
  | Some(subs) => Array.append(subs, [|subscription|])
  | None => [|subscription|]
  };
};

[@react.component]
let make = () => {
  let (Hooks.{response}) =
    Hooks.useSubscription(
      ~request=SubscribeRandomInt.make(),
      ~handler=Handler(handler)
    );

  switch (response) {
    | Fetching => <div> "Loading"->React.string </div>
    | Data(d) =>
      d
      |> Array.map(
      (datum) =>
        <circle
          cx=datum##newNumber
          cy=datum##newNumber
          stroke="#222"
          fill="none"
          r="5"
        />,
      )
      |> React.array
    | Error(_e) => <div> "Error"->React.string </div>
    | NotFound => <div> "NotFound"->React.string </div>
  }
}
```

Check out `examples/5-subscription` to see an example of using the `useSubscription` hook.

## `useClient`

`useClient` allows you to access your `urql` client instance anywhere within your React component tree.

### Return Type

| Return Value | Type       | Description                                      |
| ------------ | ---------- | ------------------------------------------------ |
| `client`     | `Client.t` | The `urql` client instance for your application. |

### Example

```reason
open ReasonUrql;

module GetAllDogs = [%graphql {|
  query dogs {
    dogs {
      name
      breed
      likes
    }
  }
|}];

[@react.component]
let make = () => {
  let client = Hooks.useClient();

  React.useEffect1(() => {
    let subscription = Client.executeQuery(~client, ~request=GetAllDogs.make(), ())
      |> Wonka.subscribe((. result) => Js.log(result));

    Some(subscription.unsubscribe);
  }, [|client|]);

  <div> "You can now use your client!"->React.string </div>
}
```